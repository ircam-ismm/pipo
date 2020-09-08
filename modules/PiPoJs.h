/** -*-mode:c; c-basic-offset: 2; -*-
 *
 * @file PiPoJs.h
 * @author Diemo.Schwarz@ircam.fr
 *
 * @brief PiPo that evaluates javascript 
 *
 * Copyright (C) 2020 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 *
 */
#ifndef _PIPO_JS_
#define _PIPO_JS_

#include <exception>
#include <sstream>
#include <string>

#include "PiPo.h"
#include "jerryscript.h"
#include "jerryscript-ext/handler.h"


// jerryscript multi-context solution (for completely independent interpreters):
// Keep pointers to the current context, must be set through callbacks below after context switch, before using any library function
// Note that it is a thread-local variable, and is hopefully thread safe.
__thread jerry_context_t *current_context_p = NULL;

// Set the current_context_p as the passed pointer.
static void jerry_port_set_current_context (jerry_context_t *context_p) /**< points to the created context */
{
  current_context_p = context_p;
}

// Get the current context.
jerry_context_t *jerry_port_get_current_context (void)
{
  return current_context_p;
}

  
class PiPoJs : public PiPo
{
public:
  PiPoScalarAttr<const char *>  expr_attr_;
  PiPoScalarAttr<const char *>  label_expr_attr_;
  PiPoVarSizeAttr<float>	param_attr_;

private:
  std::vector<PiPoValue> buffer_;
  unsigned int           inframesize_  = 0;    // cache max input frame size
  unsigned int           outframesize_ = 0;    // frame size to be produced
  jerry_context_t	*jscontext_;
  jerry_value_t		 global_object_;
  jerry_value_t		 parsed_expr_;
  typedef enum { scalar, array, typedarray, other } output_type_t;
  output_type_t		 output_type_;	// return type of parsed_expr_

  //jerry_value_t	 input_frame_;	// data frame object to be input to script
  jerry_value_t		 input_array_;  // array object "a" for input data frame when using expr
  jerry_value_t		 param_array_;  // array object "p" for external params when using expr
  jerry_value_t		 labels_obj_;   // object "c" for input data column labels

  std::map<jerry_error_t, const char *> error_name_ = {
    {JERRY_ERROR_COMMON,    "common error"}, 
    {JERRY_ERROR_EVAL,	    "eval error"}, 
    {JERRY_ERROR_RANGE,     "range error"}, 
    {JERRY_ERROR_REFERENCE, "reference error"}, 
    {JERRY_ERROR_SYNTAX,    "syntax error"}, 
    {JERRY_ERROR_TYPE,      "type error"}, 
    {JERRY_ERROR_URI,       "URI error"} 
  };

  /* Allocate JerryScript heap for each thread. */
  static void *jscontext_alloc_fn (size_t size, void *cb_data)
  {
    (void) cb_data;
    return malloc(size);
  }

public:
  PiPoJs (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    expr_attr_       (this, "expr", "JS expression producing output frame array from input in array a", true, ""),
    label_expr_attr_ (this, "labelexpr", "JS expression producing stream label array from input labels in array l", true, ""),
    param_attr_      (this, "p",    "Parameter array p for JS expression", false)
  {
    //printf("PiPoJs %p ctor\n", this);

    try {
      // Initialize engine in context
      jscontext_ = jerry_create_context (512 * 1024, jscontext_alloc_fn, NULL);
      jerry_port_set_current_context(jscontext_);
      jerry_init(JERRY_INIT_EMPTY);

      /* Register 'print' function from the extensions to the global object */
      jerryx_handler_register_global ((const jerry_char_t *) "print", jerryx_handler_print);

      /* Getting pointer to the Global object */
      global_object_ = jerry_get_global_object ();

      // init js objects to undefined
      parsed_expr_ = jerry_create_error(JERRY_ERROR_TYPE, (const jerry_char_t *) "no expression");
      input_array_ = jerry_create_undefined();
      param_array_ = jerry_create_undefined();
      labels_obj_  = jerry_create_undefined();

      // quick map of external functions (name -> handler)
      struct {
	const char *name;
	jerry_value_t (*handler) (const jerry_value_t function_object, const jerry_value_t function_this,
				  const jerry_value_t arguments[], const jerry_length_t argument_count);
      } external_functions[] =
      {
	{ "mtof",  mtof_handler },
	{ "ftom",  ftom_handler },
	{ "atodb", atodb_handler },
	{ "dbtoa", dbtoa_handler }
      };

      for (size_t i = 0; i < sizeof(external_functions) / sizeof(external_functions[0]); i++)
      {
	// Create a name JS string
	jerry_value_t property_name  = jerry_create_string((const jerry_char_t *) external_functions[i].name);
	// Create function from native C method (this function will be called from JS)
	jerry_value_t property_value = jerry_create_external_function(external_functions[i].handler);
	// Add the property with the function value to the "global" object
	jerry_value_t set_result = jerry_set_property(global_object_, property_name, property_value);

	// Check if there was no error when adding the property (in this case it should never happen)
	if (jerry_value_is_error(set_result))
	  throw std::logic_error("Failed to add the function property");

	// Release all jerry_values
	jerry_release_value (set_result);
	jerry_release_value (property_value);
	jerry_release_value (property_name);
      }
    }
    catch (std::exception &e)
    {
      printf("ARGH!!!! PiPoJs ctor caught: %s\n", e.what());
    }
  }

  ~PiPoJs (void)
  {
    //printf("PiPoJs %p dtor\n", this);
    jerry_port_set_current_context(jscontext_);

    /* Releasing the Global object */
    jerry_release_value(global_object_);    
    jerry_release_value(parsed_expr_);
    jerry_release_value(input_array_);
    jerry_release_value(param_array_);
    jerry_release_value(labels_obj_);

    // Cleanup engine in context (must do this only once)
    jerry_cleanup();
    free(jscontext_);
  }

  
/****************************************
 *
 * helper functions
 *
 */

private:
  void set_property (jerry_value_t obj, const char *name, jerry_value_t prop) throw()
  {
    jerry_value_t prop_name  = jerry_create_string((const jerry_char_t *) name);
    jerry_value_t set_result = jerry_set_property(obj, prop_name, prop);
    bool iserror = jerry_value_is_error(set_result);
    jerry_release_value(set_result);
    jerry_release_value(prop_name);

    if (iserror)
      throw std::logic_error(std::string("Failed to set property '") + name); 
  }

  // create array and set as obj.name
  jerry_value_t create_array (jerry_value_t obj, const char *name, int size) throw()
  {
    jerry_value_t a_arr = jerry_create_typedarray(JERRY_TYPEDARRAY_FLOAT32, size);
    set_property(obj, name, a_arr);
    return a_arr;
  }

  // set values of float array object from pointer
  void set_array (jerry_value_t arr, size_t size, PiPoValue *data) throw()
  {
    // set using arraybuffer
    jerry_length_t bytelength = 0;
    jerry_length_t byteoffset = 0;
    jerry_value_t  buffer = jerry_get_typedarray_buffer(arr, &byteoffset, &bytelength);

    if (bytelength != size * sizeof(PiPoValue))
    {
      char msg[1024];
      snprintf(msg, 1023, "set_array: unexpected array size %g instead of %lu", (float) bytelength / sizeof(PiPoValue), size);
      jerry_release_value(buffer);
      
      throw std::logic_error(msg);
    }

    jerry_arraybuffer_write(buffer, byteoffset, (uint8_t *) data, bytelength);
    jerry_release_value(buffer);
  }
  
  jerry_value_t create_frame (int size) throw ()
  {
    jerry_value_t frm_obj = jerry_create_object();
    create_array(frm_obj, "data", size);
    set_property (global_object_, "frm", frm_obj);
    return frm_obj;
  }
  
  const std::string value_to_string (jerry_value_t errval, const std::string &defval = "")
  {
    std::string errmsg = defval;
    
    if (jerry_value_is_string(errval))
    {
      size_t errlen = jerry_get_string_size(errval); // including terminating 0
      errmsg.resize(errlen);
      jerry_string_to_char_buffer(errval, (jerry_char_t*) &errmsg[0], errlen);
    }
    return errmsg;
  }

  void check_error (jerry_value_t value, const std::string &message, bool release = false)
  {
    if (jerry_value_is_error(value))
    {
      jerry_error_t errtype = jerry_get_error_type(value);
      jerry_value_t errval  = jerry_get_value_from_error(value, false);
      std::string errmsg = value_to_string(errval, "(no message)");
      jerry_release_value(errval);
      if (release) jerry_release_value(value);
      throw std::logic_error(message +": "+ error_name_[errtype] +" '"+ errmsg +"'");
    }
  }
	
/****************************************
 *
 * useful functions made available in js
 *
 */

public:
  static double mtof(double x) { double ref = 440; return ref * exp(0.0577622650467 * (x - 69.0)); }
  static double ftom(double x) { double ref = 440; return 69.0 + 17.3123404906676 * log(x / ref); }
  static double atodb(double x) { return (x) <= 0.000000000001  ?   -240.0  :  8.68588963807 * log(x); }
  static double dbtoa(double x) { return exp(0.11512925465 * x); }

#define CREATE_HANDLER(func)						\
  static jerry_value_t func ## _handler (const jerry_value_t function_object, \
					 const jerry_value_t function_this, \
					 const jerry_value_t arguments[], \
					 const jerry_length_t argument_count) \
  {									\
    if (argument_count > 0  &&  jerry_value_is_number(arguments[0]))	\
    {									\
      double ret = func(jerry_get_number_value(arguments[0]));		\
      return jerry_create_number(ret);					\
    }									\
    else								\
      return jerry_create_undefined ();					\
  }
  CREATE_HANDLER(mtof)
  CREATE_HANDLER(ftom)
  CREATE_HANDLER(atodb)
  CREATE_HANDLER(dbtoa)

	  
/****************************************
 *
 * pipo API methods
 *
 */

public:
  /* Configure PiPo module according to the input stream attributes and propagate output stream attributes.
   * Note: For audio input, one PiPo frame corresponds to one sample frame, i.e. width is the number of channels, height is 1, maxFrames is the maximum number of (sample) frames passed to the module, rate is the sample rate, and domain is 1 / sample rate.
   */
  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int height,
                        const char **labels, bool hasVarSize,
                        double domain, unsigned int maxFrames)
  {
    printf("PiPoJs %p streamAttributes:\n", this); /* %s\n", this,
	   PiPoStreamAttributes(hasTimeTags, rate, offset, width, height,
	   labels, hasVarSize, domain, maxFrames).to_string().c_str()); */

    inframesize_ = width * height; // we need to store the max frame size in case hasVarSize is true
    int outwidth = width, outheight = height;
    bool outlabels_given = false;       // default: pass labels through
    std::vector<const char *> outlabelarr; // output labels pointer array, if changed
    std::vector<std::string>  outlabelstr; // stores output of label expression

    try {
      const char *expr_str = expr_attr_.getStr(0);
      size_t	  expr_len = strlen(expr_str);

      if (expr_len == 0)
	// no expression given
	throw std::logic_error("no expr given");
      
      jerry_port_set_current_context(jscontext_);

      // parse and run label expression
      const char *label_expr_str = label_expr_attr_.getStr(0);
      size_t	  label_expr_len = strlen(label_expr_str);
	
      if (label_expr_len > 0)
      { // label expr is given, parse, run, retrieve output label list
	jerry_value_t parsed_label_expr = jerry_parse(NULL, 0, (const jerry_char_t *) label_expr_str, label_expr_len, JERRY_PARSE_NO_OPTS);
	check_error(parsed_label_expr, std::string("can't parse label js expression '") + label_expr_str +"'", true);

	// run label expr
	jerry_value_t ret_value = jerry_run(parsed_label_expr);
	jerry_release_value(parsed_label_expr);

	// determine return type: string or untyped array (no typed array for string)
	if (jerry_value_is_string(ret_value))
	{
	  outlabelstr.resize(1);
	  outlabelstr[0] = value_to_string(ret_value);
	}
	else if (jerry_value_is_array(ret_value))
	{
	  size_t numlabels = jerry_get_array_length(ret_value);
	  outlabelstr.resize(numlabels);

	  for (unsigned int j = 0; j < numlabels; j++)
	  {
	    jerry_value_t elem = jerry_get_property_by_index(ret_value, j);
	    outlabelstr[j] = value_to_string(elem, "");
	    jerry_release_value(elem);
	  }
	}
	else if (jerry_value_is_error(ret_value))
	{ // error
	  jerry_release_value(ret_value);
	  throw std::logic_error("error evaluating labelexpr to determine output labels");
	}
	else
	{ // wrong type
	  jerry_release_value(ret_value);
	  throw std::logic_error("wrong label expr return type");
	}
	jerry_release_value(ret_value);

	// copy to string array
	outlabelarr.resize(outlabelstr.size());
	for (int i = 0; i < outlabelstr.size(); i++)
	  outlabelarr[i] = outlabelstr[i].c_str();

	outlabels_given = true;
      } // end label expr
		
      // create js array "a" for pipo input, set to 0
      jerry_release_value(input_array_); // have to release previous value
      input_array_ = create_array(global_object_, "a", inframesize_);
      std::vector<PiPoValue> zeros(inframesize_);
      set_array(input_array_, inframesize_, zeros.data());

      // create js array "p" for pipo params, set to current values of param_attr_
      jerry_release_value(param_array_); // have to release previous value
      param_array_ = create_array(global_object_, "p", param_attr_.size());
      set_array(param_array_, param_attr_.size(), param_attr_.getPtr());

      // create js obj "c" with input column labels and their indices to be used in expr	
      jerry_release_value(labels_obj_); // have to release previous value
      labels_obj_ = jerry_create_object();
      set_property(global_object_, "c", labels_obj_);
      if (labels != NULL)
	for (int i = 0; i < width; i++)
	{
	  if (labels[i] != NULL  &&  *labels[i] != 0) // todo: check if valid js identifier
	  {
	    jerry_value_t index = jerry_create_number(i);
	    set_property(labels_obj_, labels[i], index);
	    jerry_release_value(index);
	  }
	}

      // parse and run frame expression once to determine output frame size
      jerry_release_value(parsed_expr_); // have to release previous value
      parsed_expr_ = jerry_parse(NULL, 0, (const jerry_char_t *) expr_str, expr_len, JERRY_PARSE_NO_OPTS);
      check_error(parsed_expr_, std::string("can't parse js expression '") + expr_str +"'");
      jerry_value_t ret_value = jerry_run(parsed_expr_);

      // determine return type: scalar, untyped array, typed array, output dims and labels     
      if (jerry_value_is_number(ret_value))
      {
	outwidth = 1;
	outheight = 1;
	output_type_ = scalar;
      }
      else if (jerry_value_is_array(ret_value))
      {
	uint32_t len = jerry_get_array_length(ret_value);
	if (len != inframesize_)
	{ // different output size: interpret as row
	  outwidth = len;
	  outheight = 1;
	} // else: same size, pass width, height, labels unchanged
	output_type_ = array;
      }
      else if (jerry_value_is_typedarray(ret_value))
      {
	uint32_t len = jerry_get_typedarray_length(ret_value);
	if (len != inframesize_)
	{ // different output size: interpret as row
	  outwidth = len;
	  outheight = 1;
	} // else: same size, pass width, height, labels unchanged
	output_type_ = typedarray;
      }
      else if (jerry_value_is_error(ret_value))
      { // error
	output_type_ = other;
	jerry_release_value(ret_value);
	throw std::logic_error("error evaluating expr to determine output frame size");
      }
      else
      { // wrong type
	output_type_ = other;
	jerry_release_value(ret_value);
	throw std::logic_error("wrong expr return type");
      }

      jerry_release_value(ret_value);
      
      // check column labels
      if (outlabels_given)
      { // output labels are given
	outlabelarr.resize(outwidth, ""); // resize to actually used columns, just in case outwidth is greater than label size
	labels = &outlabelarr[0];	
      }
      else if (outwidth != width)
      { // no new output labels given but width changed: invalidate labels (don't pass labels but anonymous columns)
	labels = NULL;
      } // else: no output labels given, same width: pass on input labels
      
      // A general pipo can not work in place, we need to create an output buffer
      buffer_.resize(width * height * maxFrames);
    }
    catch (std::exception &e)
    {
      printf("streamAttributes caught: %s\n", e.what());
      signalError(e.what());
      return -1;
    }
    
    // pass on produced stream layout
    outframesize_ = outwidth * outheight;
    return propagateStreamAttributes(hasTimeTags, rate, offset, outwidth, outheight,
                                     labels, hasVarSize, domain, maxFrames);
  } // end streamAttributes()

  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    PiPoValue *outptr = &buffer_[0];

    try {
      jerry_port_set_current_context(jscontext_);
      
      for (unsigned int i = 0; i < num; i++)
      {
	if (!jerry_value_is_error(parsed_expr_))
	{ // evaluate single expression
	  // set arr "a" from frame input
	  set_array(input_array_, size, values);

	  // set p to current values of param_attr_ only when changed
	  if (param_attr_.hasChanged())
	  {
	    if (param_attr_.size() != jerry_get_array_length(param_array_))
	    { // user wants to change size, need to free and recreate array
	      jerry_release_value(param_array_); // have to release previous value
	      param_array_ = create_array(global_object_, "p", param_attr_.size());
	    }
	    
	    set_array(param_array_, param_attr_.size(), param_attr_.getPtr());
	    param_attr_.resetChanged();
	  }

	  // run expr
	  jerry_value_t ret_value = jerry_run(parsed_expr_);
	  
	  switch (output_type_)
	  {
	    case array:
	    {
#if DEBUG
	      // paranoid check:
	      if (jerry_get_array_length(ret_value) != outframesize_)
	      {
		char msg[1024];
		snprintf(msg, 1023, "read: unexpected array size %d instead of %u", jerry_get_array_length(ret_value), outframesize_);
		jerry_release_value(ret_value);
		throw std::runtime_error(msg);
	      }
#endif     
	      for (unsigned int j = 0; j < outframesize_; j++)
	      {
		jerry_value_t elem = jerry_get_property_by_index(ret_value, j);
		
		if (jerry_value_is_number(elem))
		  outptr[j] = jerry_get_number_value(elem);
		else
		  outptr[j] = 0;

		jerry_release_value(elem);
	      }
	    }
	    break;
	  
	    case typedarray:
	    {
	      jerry_length_t bytelength = 0;
	      jerry_length_t byteoffset = 0;
#if DEBUG
	      // paranoid check:
	      if (jerry_get_typedarray_length(ret_value) != outframesize_)
	      {
		char msg[1024];
		snprintf(msg, 1023, "read: unexpected array size %d instead of %u", jerry_get_typedarray_length(ret_value), outframesize_);
		jerry_release_value(ret_value);
		throw std::runtime_error(msg);
	      }
#endif     
	      jerry_value_t buffer = jerry_get_typedarray_buffer(ret_value, &byteoffset, &bytelength);
	      jerry_arraybuffer_read(buffer, byteoffset, (uint8_t *) outptr, bytelength);
	      jerry_release_value(buffer);
	    }
	    break;
	  
	    case scalar:
	    {
	      outptr[0] = jerry_get_number_value(ret_value);
	    }
	    break;
	  
	    default: // error
	      jerry_release_value(ret_value);
	      throw std::runtime_error("wrong expr return type");
	      break;
	  }
	  jerry_release_value(ret_value);
	}
	
	outptr += outframesize_;
	values += size;
      } // end for 
    }
    catch (std::exception &e)
    {
      printf("frames method caught: %s\n", e.what());
      signalError(e.what());
      return -1;
    }
    catch (...)
    {
      printf("frames method caught unknown exception\n");
      signalError("unknown exception");
      return -1;
    }
    
    return propagateFrames(time, weight, &buffer_[0], outframesize_, num);
  } // frames
};

#endif // _PIPO_JS_

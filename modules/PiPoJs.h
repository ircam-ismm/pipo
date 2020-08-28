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
#include "jerryscript-port.h"
//#include "jerryscript-ext/handler.h"
#include "handler.h"

  
class PiPoJs : public PiPo
{
private:
  std::vector<PiPoValue> buffer_;
  unsigned int           framesize_ = 0;    // cache max frame size
  unsigned int           outframesize_ = 0;    // cache max frame size
  static bool		 jerry_initialized;
  jerry_context_t	*jscontext_;
  jerry_value_t		 global_object_;
  jerry_value_t		 parsed_expr_;
  typedef enum { scalar, array, typedarray, other } output_type_t;
  output_type_t		 output_type_;	// return type of parsed_expr_

  //jerry_value_t	 input_frame_;	// data frame object to be input to script
  jerry_value_t		 input_array_;   // array object "a" for input data frame when using expr
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
    printf("jscontext_alloc_fn size %d\n", size);
    (void) cb_data;
    return malloc(size);
  }

public:
  PiPoScalarAttr<const char *> expr_attr_;

  PiPoJs (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    expr_attr_(this, "expr", "JS expression producing output frame from input in array a", false, "")
  {
    printf("PiPoJs %p ctor\n", this);

    try {
      // Initialize engine in context
      jscontext_ = jerry_create_context (512 * 1024, jscontext_alloc_fn, NULL);
      jerry_init(JERRY_INIT_EMPTY);

      /* Register 'print' function from the extensions to the global object */
      jerryx_handler_register_global ((const jerry_char_t *) "print", jerryx_handler_print);

      /* Getting pointer to the Global object */
      global_object_ = jerry_get_global_object ();

      // init js objects to undefined
      parsed_expr_ = jerry_create_error(JERRY_ERROR_TYPE, (const jerry_char_t *) "no expression");
      input_array_ = jerry_create_undefined();

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

      for (int i = 0; i < sizeof(external_functions) / sizeof(external_functions[0]); i++)
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
    printf("PiPoJs %p dtor\n", this);

    /* Releasing the Global object */
    jerry_release_value(global_object_);    
    jerry_release_value(parsed_expr_);
    jerry_release_value(input_array_);

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

  void set_array (jerry_value_t arr, size_t size, PiPoValue *data)
  {
    // set using arraybuffer
    jerry_length_t bytelength = 0;
    jerry_length_t byteoffset = 0;
    jerry_value_t  buffer = jerry_get_typedarray_buffer(arr, &byteoffset, &bytelength);

    if (bytelength != size * sizeof(PiPoValue))
    {
      char msg[1024];
      snprintf(msg, 1023, "set_array: unexpected array size %f instead of %lu", (float) bytelength / sizeof(PiPoValue), size);
      
      throw std::logic_error(msg);
    }

    int bytes_written = jerry_arraybuffer_write(buffer, byteoffset, (uint8_t *) data, bytelength);
    jerry_release_value (buffer);
  }
  
  jerry_value_t create_frame (int size) throw ()
  {
    jerry_value_t frm_obj = jerry_create_object();
    create_array(frm_obj, "data", size);
    set_property (global_object_, "frm", frm_obj);
    return frm_obj;
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

    try {
      // we need to store the max frame size in case hasVarSize is true
      framesize_ = width * height; 

      const char *expr_str = expr_attr_.getStr(0);
      size_t	  expr_len = strlen(expr_str);

      if (expr_len > 0)
      {
	jerry_release_value(parsed_expr_); // have to release previous value
        parsed_expr_ = jerry_parse(NULL, 0, (const jerry_char_t *) expr_str, expr_len, JERRY_PARSE_NO_OPTS);

	if (jerry_value_is_error(parsed_expr_))
	{
	  jerry_error_t errtype = jerry_get_error_type(parsed_expr_);
	  jerry_value_t errval  = jerry_get_value_from_error(parsed_expr_, false);
	  std::string   errmsg = "(no message)";
	  
	  if (jerry_value_is_string(errval))
	  {
	    size_t errlen = jerry_get_string_size(errval);
	    unsigned char errbuf[errlen];
	    jerry_string_to_char_buffer(errval, errbuf, errlen);
	    errmsg = (const char *) errbuf;
	  }
	  jerry_release_value(errval);
	  throw std::logic_error(std::string("can't parse js expression '") + expr_str +"': "+ error_name_[errtype] +" '"+ errmsg +"'");
	}
	
	// create js array for pipo input
	jerry_release_value(input_array_); // have to release previous value
	input_array_ = create_array(global_object_, "a", framesize_);

	// set arr to 0
	std::vector<PiPoValue> zeros(framesize_);
	set_array(input_array_, framesize_, zeros.data());

	// run expr once to determine output size
	jerry_value_t ret_value = jerry_run(parsed_expr_);

	// determine return type: scalar, untyped array, typed array
	if (jerry_value_is_number(ret_value))
	{
	  width = 1;
	  height = 1;
	  output_type_ = scalar;
	}
	else if (jerry_value_is_array(ret_value))
	{
	  uint32_t len = jerry_get_array_length(ret_value);
	  if (len != framesize_)
	  { // different output size: interpret as row
	    width = len;
	    height = 1;
	    labels = NULL; // no labels, TODO: use attr
	  } // else: same size, pass width, height, labels unchanged
	  output_type_ = array;
	}
	else if (jerry_value_is_typedarray(ret_value))
	{
	  uint32_t len = jerry_get_typedarray_length(ret_value);
	  if (len != framesize_)
	  { // different output size: interpret as row
	    width = len;
	    height = 1;
	    labels = NULL; // no labels, TODO: use attr
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
      }
      else
      { // no expression given
	throw std::logic_error("no expr given");
      }
      
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
    outframesize_ = width * height;
    return propagateStreamAttributes(hasTimeTags, rate, offset, width, height,
                                     labels, hasVarSize, domain, maxFrames);
  } // streamAttributes

  
  int frames (double time, double weight, PiPoValue *values,
              unsigned int size, unsigned int num)
  {
    PiPoValue *outptr = &buffer_[0];

    try {
      for (unsigned int i = 0; i < num; i++)
      {
	if (!jerry_value_is_error(parsed_expr_))
	{ // evaluate single expression
	  // set arr "a"
	  set_array(input_array_, size, values);
	  
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
		throw std::runtime_error(msg);
	      }
#endif     
	      for (int j = 0; j < outframesize_; j++)
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
		throw std::runtime_error(msg);
	      }
#endif     
	      jerry_value_t buffer = jerry_get_typedarray_buffer(ret_value, &byteoffset, &bytelength);
	      int bytes_read = jerry_arraybuffer_read(buffer, byteoffset, (uint8_t *) outptr, bytelength);
	      jerry_release_value(buffer);
	    }
	    break;
	  
	    case scalar:
	    {
	      outptr[0] = jerry_get_number_value(ret_value);
	    }
	    break;
	  
	    default: // error
	      throw std::runtime_error("wrong expr return type");
	      break;
	  }
	  jerry_release_value(ret_value);
	}
	
	outptr += outframesize_;
	values += size;
      }
    }
    catch (std::exception &e)
    {
      printf("frames method caught: %s\n", e.what());
      signalError(e.what());
      return -1;
    }
    
    return propagateFrames(time, weight, &buffer_[0], outframesize_, num);
  } // frames
};

bool PiPoJs::jerry_initialized = false;

#endif // _PIPO_JS_

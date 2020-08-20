/* https://github.com/jerryscript-project/jerryscript/blob/master/docs/03.API-EXAMPLE.md#example-3-split-javascript-parsing-and-script-execution
   compile with:
   clang pipo-js-test1.cpp -I ../../modules/javascript-engine/lib/include/ -L ../../modules/javascript-engine/lib/lib/ -ljerry-core -ljerry-ext -ljerry-port-default 
*/

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"

int
main (void)
{
  bool run_ok = false;

  const jerry_char_t script[] = "print ('Hello from JS with ext!'); print(my_var);";

  /* Initialize engine */
  jerry_init (JERRY_INIT_EMPTY);

  /* Register 'print' function from the extensions to the global object */
  jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);

  {
  /* Getting pointer to the Global object */
  jerry_value_t global_object = jerry_get_global_object ();

  /* Constructing strings */
  jerry_value_t prop_name = jerry_create_string ((const jerry_char_t *) "my_var");
  jerry_value_t prop_value = jerry_create_string ((const jerry_char_t *) "Hello from C!");

    /* Setting the string value as a property of the Global object */
  jerry_value_t set_result = jerry_set_property (global_object, prop_name, prop_value);
  /* The 'set_result' should be checked if there was any error */
  if (jerry_value_is_error (set_result)) {
    printf ("Failed to add the 'my_var' property\n");
  }
  jerry_release_value (set_result);

  /* Releasing string values, as it is no longer necessary outside of engine */
  jerry_release_value (prop_name);
  jerry_release_value (prop_value);
  
  /* Releasing the Global object */
  jerry_release_value (global_object);
  }

  
  /* Setup Global scope code */
  jerry_value_t parsed_code = jerry_parse (NULL, 0, script, sizeof (script) - 1, JERRY_PARSE_NO_OPTS);

  /* Check if there is any JS code parse error */
  if (!jerry_value_is_error (parsed_code))
  {
    /* Execute the parsed source code in the Global scope */
    jerry_value_t ret_value = jerry_run (parsed_code);

    /* Check the execution return value if there is any error */
    run_ok = !jerry_value_is_error (ret_value);

    /* Returned value must be freed */
    jerry_release_value (ret_value);
  }

  /* Parsed source code must be freed */
  jerry_release_value (parsed_code);

  /* Cleanup engine */
  jerry_cleanup ();

  return (run_ok ? 0 : 1);
}

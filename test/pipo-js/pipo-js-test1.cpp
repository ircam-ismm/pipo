/* https://github.com/jerryscript-project/jerryscript/blob/master/docs/03.API-EXAMPLE.md#example-3-split-javascript-parsing-and-script-execution
   compile with:
   clang pipo-js-test1.cpp -I ../../modules/javascript-engine/lib/include/ -L ../../modules/javascript-engine/lib/lib/ -ljerry-core -ljerry-port-default 
*/

#include "jerryscript.h"

int
main (void)
{
  bool run_ok = false;

  const jerry_char_t script[] = "var str = 'Hello, World!';";

  /* Initialize engine */
  jerry_init (JERRY_INIT_EMPTY);

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

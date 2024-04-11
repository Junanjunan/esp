matrix_keyboard_4_by_4.c code description

* ### gpio_set_dicrection

    There are set directions of row gpio and column gpio like this:
      gpio_set_dicrection(row_pins[i], GPIO_MODE_OUTPUT);
      gpio_set_dicrection(col_pins[i], GPIO_MODE_INPUT);
    
    Why are they needed and,
    Why are their directions different?

    In the matrix keyboard, it determines which key is pressed with row and column combination.
    when specific row and specific column are selected, we can determine what key is pressed.

    The keyboard use current of electricity.
    In the matrix keyboard, one side sends current, the opposite side receives current.
    The one side can be output or input and opposite side can be input or output.
      If I set the row output mode, I can manipulate output row's state.
      If I set the column input mode, input column can read voltage levels.
    Now, with manipulated output row's state(high or low voltage) and input voltage level, I can handle several situation.

    On the code, pull mode is pull up about column. So column acts like 3.3V.
    When I make output row voltage 0 with gpio_set_level(row[i], 0), now it acts like GND.
    So, when I press button, current can flow from 3.3V(input column) to GND(output row).
    So In that code, rows and columns have opposite INPUT, OUTPUT mode.

    (If you have confusion about ouput and input. Read this "GPIO Default State".)

<br>

* ### GPIO Default State

    Upon microcontroller reset, GPIO pins are typically initialized to a default state,
    which often means they are configured as inputs, not outputs.
    The exact default state can vary depending on the microcontroller.

    As inputs, their primary function is to read voltage levels,
    not to drive them to a specific state.

    -> If user want to control state of pin, that pin has to be output.
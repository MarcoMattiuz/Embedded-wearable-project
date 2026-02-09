# LCD Task

The `LCD_task` is a FreeRTOS task responsible for controlling the OLED display,
handling UI state transitions, and reacting to system events.

## task start up
```c
    esp_lcd_panel_handle_t panel_handle = *(esp_lcd_panel_handle_t *)parameters;

    //init queue,isr and timers
    GPIO_init();
    memset(buffer_data, 0, sizeof(buffer_data));

    EventType evt;

    //disable push button interrupt during loading screen
    gpio_intr_disable(PUSH_BUTTON_GPIO);


    bool LCD_ON = true;
    TurnLcdOn(panel_handle);
    show_loading_screen(&panel_handle);

    //after loading screen go to BPM screen
    xTimerStart(refresh_timer_handle, 0);
    global_parameters.show_heart = true;
    (*fsm[current_state].state_function)(&panel_handle, &global_parameters);
    xTimerStart(frame_timer_handle, 0);

    //and clear queue and re-enable button interrupt 
    xQueueReset(event_queue);
    gpio_intr_enable(PUSH_BUTTON_GPIO);
```
Queue, isr and timers are initialized and loading screen is shown.

## Event Handling
```c
    while (1)
    {
        if (xQueueReceive(event_queue, &evt, portMAX_DELAY))
        {
            switch (evt)
            {
            case EVT_GYRO:
            {
                LCD_ON = false;
                if (current_state == STATE_BPM)
                {
                    xTimerStop(frame_timer_handle, 0);
                }
                ESP_LOGE(TAG, "LCD OFF GYRO");
                TurnLcdOff(panel_handle);
                break;
            }
            case EVT_BUTTON_EDGE:
            {
                int level = gpio_get_level(PUSH_BUTTON_GPIO);
                if (level == 0) // pressed
                {
                    long_press_triggered = false;
                    xTimerStart(long_press_timer_handle, 0);
                }
                else // released
                {
                    xTimerStop(long_press_timer_handle, 0);
                    if (!long_press_triggered && LCD_ON) // short press
                    {
                        ESP_LOGE(TAG, "LCD NEXT STATE");
                        (*fsm[next_state].state_function)(&panel_handle, &global_parameters);

                        current_state = next_state;
                        next_state = get_next_state(current_state);

                        // Start/stop BPM animation if on BPM screen
                        if (current_state == STATE_BPM)
                        {
                            xTimerStart(frame_timer_handle, 0);
                        }
                        else
                        {
                            xTimerStop(frame_timer_handle, 0);
                        }
                    }
                }
                break;
            }

            case EVT_LONG_PRESS:
            {
                if (gpio_get_level(PUSH_BUTTON_GPIO) != 0)
                {
                    // Ignore stale long-press event if the button was already released.
                    break;
                }
                long_press_triggered = true;
                LCD_ON = !LCD_ON;

                if (LCD_ON)
                {
                    xTimerStart(refresh_timer_handle, 0);
                    TurnLcdOn(panel_handle);
                    ESP_LOGE(TAG, "LCD ON LONG PRESS");
                    current_state = STATE_BPM;
                    next_state = get_next_state(current_state);

                    global_parameters.show_heart = true;
                    (*fsm[current_state].state_function)(&panel_handle, &global_parameters);

                    xTimerStart(frame_timer_handle, 0); // start animation
                }
                else
                {
                    TurnLcdOff(panel_handle);
                    ESP_LOGE(TAG, "LCD OFF LONG PRESS");
                    xTimerStop(refresh_timer_handle, 0);
                    xTimerStop(frame_timer_handle, 0); // stop animation
                }
                break;
            }

            case EVT_REFRESH:
                (*fsm[current_state].state_function)(&panel_handle, &global_parameters);
                ESP_LOGE(TAG, "LCD REFRESH");
                break;

            default:
                break;
            }
        }
    }
```

### EVT_GYRO
```c
case EVT_GYRO:
            {
                LCD_ON = false;
                if (current_state == STATE_BPM)
                {
                    xTimerStop(frame_timer_handle, 0);
                }
                ESP_LOGE(TAG, "LCD OFF GYRO");
                TurnLcdOff(panel_handle);
                break;
            }
```

Turns the LCD off when motion is detected.

Stops the BPM animation if the current state is STATE_BPM

### EVT_BUTTON_EDGE
```c
case EVT_BUTTON_EDGE:
            {
                int level = gpio_get_level(PUSH_BUTTON_GPIO);
                if (level == 0) // pressed
                {
                    long_press_triggered = false;
                    xTimerStart(long_press_timer_handle, 0);
                }
                else // released
                {
                    xTimerStop(long_press_timer_handle, 0);
                    if (!long_press_triggered && LCD_ON) // short press
                    {
                        ESP_LOGE(TAG, "LCD NEXT STATE");
                        (*fsm[next_state].state_function)(&panel_handle, &global_parameters);

                        current_state = next_state;
                        next_state = get_next_state(current_state);

                        // Start/stop BPM animation if on BPM screen
                        if (current_state == STATE_BPM)
                        {
                            xTimerStart(frame_timer_handle, 0);
                        }
                        else
                        {
                            xTimerStop(frame_timer_handle, 0);
                        }
                    }
                }
                break;
            }

```
Handles push-button press and release.
- Button Pressed:
    Starts the long-press timer.
    Resets the long_press_triggered flag.
- Button Released
    Stops the long-press timer.
    If no long press occurred and the LCD is on:
        Switch to the next UI state -> Executes the corresponding state function.
    Starts or stops the frame timer depending on whether the new state is STATE_BPM.
### EVT_LONG_PRESS
```c
case EVT_LONG_PRESS:
            {
                if (gpio_get_level(PUSH_BUTTON_GPIO) != 0)
                {
                    // Ignore stale long-press event if the button was already released.
                    break;
                }
                long_press_triggered = true;
                LCD_ON = !LCD_ON;

                if (LCD_ON)
                {
                    xTimerStart(refresh_timer_handle, 0);
                    TurnLcdOn(panel_handle);
                    ESP_LOGE(TAG, "LCD ON LONG PRESS");
                    current_state = STATE_BPM;
                    next_state = get_next_state(current_state);

                    global_parameters.show_heart = true;
                    (*fsm[current_state].state_function)(&panel_handle, &global_parameters);

                    xTimerStart(frame_timer_handle, 0); // start animation
                }
                else
                {
                    TurnLcdOff(panel_handle);
                    ESP_LOGE(TAG, "LCD OFF LONG PRESS");
                    xTimerStop(refresh_timer_handle, 0);
                    xTimerStop(frame_timer_handle, 0); // stop animation
                }
                break;
            }

```
Toggles display on/off.

Safety Check: Ignores stale events if the button has already been released (long-press timer expires *after* the user has already released the button)

If LCD turns ON:
    Starts the refresh timer.
    Powers on the display.
    Resets the UI to STATE_BPM.
    Executes the BPM state function.
    Starts the animation timer.
If LCD turns OFF:
    Powers off the display.
    Stops both refresh and animation timers.

### EVT_REFRESH
```c
case EVT_REFRESH:
                (*fsm[current_state].state_function)(&panel_handle, &global_parameters);
                ESP_LOGE(TAG, "LCD REFRESH");
                break;

```
Re-renders the current UI state by invoking its state function.

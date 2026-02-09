# display_fsm.h/c
This module implements the state machine for display UI.

Each state corresponds to a different screen (BPM, clock, weather, etc.).

It uses the global struct `global_param` as source of its data. The struct is regularly updated by other sensors

## FSM

```c
StateMachine_t fsm[] = {
    {STATE_BPM, fn_BPM},
    {STATE_CLOCK, fn_CLOCK},
    {STATE_WEATHER, fn_WEATHER},
    {STATE_STEPS, fn_STEPS},
    {STATE_BATTERY, fn_BATTERY},
    {STATE_CO2, fn_CO2},
    {STATE_PARTICULATE, fn_PARTICULATE},
};
```
Each function of the state machine draws the UI for that state.

## Events
The LCD task receives events through a queue. Depending on the event the task will redraw the display/ transition on the next state or turn on/off the display. The event source can be timers, interrupts or other tasks. 
```c
typedef enum
{
    EVT_BUTTON_EDGE,
    EVT_LONG_PRESS,
    EVT_REFRESH,
    EVT_GYRO,
} EventType;

```
# Interrupts and timers:

  void GPIO_init(void): initializes Push button GPIO, Event queue, ISR service,timers.

## Push button ISR:
```c
static void IRAM_ATTR button_isr(void *arg)
{
    uint32_t now = xTaskGetTickCountFromISR();

    // Debounce: ignore if signal comes too soon
    if ((now - last_button_isr_time) < pdMS_TO_TICKS(DEBOUNCE_MS))
    {
        return;
    }
    last_button_isr_time = now;

    EventType evt = EVT_BUTTON_EDGE;

    xQueueSendFromISR(event_queue, &evt, NULL);
}
```
Triggers when the push button is pressed and when its released. Button debouncing is used to prevent the button from being interpreted as multiple presses when it is actually pressed only once.

## Long Press Timer:
```c
void long_press_timer_handler(TimerHandle_t xTimer)
{
    EventType evt = EVT_LONG_PRESS;
    xQueueSend(event_queue, &evt, 0);
}
```
Sends EVT_LONG_PRESS when a long press of the button is detected

## Refresh Timer:
```c
void refresh_timer_handler(TimerHandle_t xTimer)
{
    EventType evt = EVT_REFRESH;
    xQueueSend(event_queue, &evt, 0);
}
```
Sends EVT_REFRESH, used to trigger periodic redraws of the display

## Frame Timer:
```c
void frame_timer_handler(TimerHandle_t xTimer)
{
    EventType evt = EVT_REFRESH;
    global_parameters.show_heart = !global_parameters.show_heart;
    xQueueSend(event_queue, &evt, 0);
}
```
Sends EVT_REFRESH and toggles "show_heart", used to perform the blinking heart when on the BPM state.
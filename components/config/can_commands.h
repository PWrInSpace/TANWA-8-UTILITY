#ifndef PWRINSPACE_CAN_COMMANDS_H
#define PWRINSPACE_CAN_COMMANDS_H

/** PLACE YOUR CAN CALLBACKS AND CAN MESSAGES HERE IN FORMAT*/
typedef enum {
    CAN_SEND_STATUS = 0x0EE0,
    CAN_UTIL_GET_STATUS_ID = 0x3EF0,
    CAN_NEW_SEND_STATUS_ID = 0x3EF9,
    CAN_BUZZER_TOGGLE = 0x0EEE,
} can_message_id_t;

/*PLACE YOUR FUNCTIONS ACCORDING TO THE TEMPLATE
* typedef esp_err_t (*can_command_handler_t)(uint8_t *data, uint8_t length);
* 
* REGISTER THEM IN can_config.c FILE
*/




#endif //PW
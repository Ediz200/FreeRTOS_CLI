/*
 * task_handler.c
 *
 *  Created on: Jul 4, 2025
 *      Author: edizt
 */
#include "main.h"

int extract_command(command_t *cmd);
void process_command(command_t *cmd);

const char *msg_inv = "\n///Invalid option///\n";

void menu_task(void* parameters)
{
	uint32_t cmd_addr;

	command_t *cmd;

	int option;


	const char *msg_menu ="\n=====================\n"
							"|        Menu       |\n"
							"=====================\n"
							"LED effect    ----> 0\n"
							"Date and time ----> 1\n"
							"Exit          ----> 2\n"
							"Enter your choice here : ";

	while(1){
		/* Send the addres of the pointer to QueueSend API*/
		xQueueSend(print_queue,(void*)&msg_menu, portMAX_DELAY);

		/* Wait for menu commands, saves the pointer uint32_t cmd_addr*/
		xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);

		/*Zet een uint32_t waarde om naar een pointer zodat je de originele struct kunt gebruiken. cmd wijst nu naar de oorspronkelijke adres*/
		cmd = (command_t*)cmd_addr;

		if(cmd->len == 1)
		{
			option = cmd->payload[0] - 48; // translate ASCI to number

			switch(option)
			{
				case 0:
					curr_state = sLedEffect;
					xTaskNotify(handle_led_task,0,eNoAction);
					//portYIELD();
					break;
				case 1:
					curr_state = sRtcMenu;
					xTaskNotify(handle_rtc_task,0,eNoAction);
					//portYIELD();
					break;
				case 2: /* Implement exit*/
					break;
				default:
					xQueueSend(print_queue,(void*)&msg_inv, portMAX_DELAY);
					continue;

			}
		} else {
			// Invalid entry
			xQueueSend(print_queue,(void*)&msg_inv, portMAX_DELAY);
			continue;
		}

		//wait to run again when some other task notifies
		xTaskNotifyWait(0,0,NULL,portMAX_DELAY);
	}

}

void command_task(void* parameters)
{
	BaseType_t xreturned;
	command_t cmd;

	while(1){

		xreturned = xTaskNotifyWait(0,0,NULL,portMAX_DELAY);
		if(xreturned == pdTRUE)
		{
			process_command(&cmd);
		}

	}
}

void process_command(command_t *cmd)
{
	extract_command(cmd);

	switch(curr_state)
	{
		case sMainMenu:
			/*xTaskNotify only accepts uint32_t. Typecast the pointer value to uint32_t*/
			xTaskNotify(handle_menu_task, (uint32_t)cmd, eSetValueWithOverwrite);
			break;

		case sLedEffect:
			xTaskNotify(handle_led_task, (uint32_t)cmd, eSetValueWithOverwrite);
			break;

		case sRtcMenu:
		case sRtcTimeConfig:
		case sRtcDataConfig:
		case sRtcReport:
			xTaskNotify(handle_rtc_task, (uint32_t)cmd, eSetValueWithOverwrite);
			break;

	}
}


int extract_command(command_t *cmd)
{
	char item;
	BaseType_t status;

	status = uxQueueMessagesWaiting(input_data_queue);
	if(!status) return -1;

	uint8_t i = 0;

	do
	{
		status = xQueueReceive(input_data_queue, &item, 0);
		if(status == pdTRUE) cmd->payload[i++] = item;

	}while(item != '\n');

	cmd->payload[i-1] = '\0';  // Replace \n with \0
	cmd->len = i - 1;

	return 0;

}

void print_task(void* parameters)
{
	uint32_t msg_addr;
	uint8_t* msg;
	while(1){
		xQueueReceive(print_queue, &msg_addr, portMAX_DELAY);
		msg = (uint8_t*)msg_addr;
		HAL_UART_Transmit(&huart2, msg, strlen((char*)msg), HAL_MAX_DELAY);
	}
}

void rtc_task(void* parameters)
{
	while(1){

	}
}

void led_task(void *param)
{
	uint32_t cmd_addr;
	command_t *cmd;
	const char* msg_led =   "=====================\n"
							"|     LED Effect    |\n"
							"=====================\n"
						    "(none,e1,e2,e3,e4)   \n"
						    "Enter your choice here : ";

	while(1){
		/*Wait for notification from menu task (Notify wait) */
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		/*Print LED menu */
		xQueueSend(print_queue,(void*)&msg_led, portMAX_DELAY);
		/*wait for LED command notify from command_task (Notify wait) */
		xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);
		cmd = (command_t*)cmd_addr;

		if(cmd->len <= 4)
		{
			if(! strcmp((char*)cmd->payload,"none"))
				led_effect_stop();
			else if (! strcmp((char*)cmd->payload,"e1"))
				led_effect(1);
			else if (! strcmp((char*)cmd->payload,"e2"))
				led_effect(2);
			else if (! strcmp((char*)cmd->payload,"e3"))
				led_effect(3);
			else if (! strcmp((char*)cmd->payload,"e4"))
				led_effect(4);
			else
				/*print invalid message */
				xQueueSend(print_queue, (void*)&msg_inv, portMAX_DELAY);
		}else {
			/*print invalid message */
			xQueueSend(print_queue,(void*)&msg_inv, portMAX_DELAY);
		}

		/*update state variable */
		curr_state = sMainMenu;

		/*Notify menu task */
		xTaskNotify(handle_menu_task,0,eNoAction);

	}
}

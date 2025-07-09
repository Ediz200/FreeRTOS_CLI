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

time_unit rtc_state = uren;
dates rtc_date_state = day_config;

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
		case sRtcDateConfig:
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


void rtc_task(void *param)
{
	const char* msg_rtc1 = "========================\n"
							"|         RTC          |\n"
							"========================\n";

	const char* msg_rtc2 = "Configure Time            ----> 0\n"
							"Configure Date            ----> 1\n"
							"Enable reporting          ----> 2\n"
							"Exit                      ----> 3\n"
							"Enter your choice here : ";


	const char *msg_rtc_hh = "Enter hour(1-12):";
	const char *msg_rtc_mm = "Enter minutes(0-59):";
	const char *msg_rtc_ss = "Enter seconds(0-59):";

	const char *msg_rtc_dd  = "Enter date(1-31):";
	const char *msg_rtc_mo  ="Enter month(1-12):";
	const char *msg_rtc_dow  = "Enter day(1-7 sun:1):";
	const char *msg_rtc_yr  = "Enter year(0-99):";

	const char *msg_conf = "Configuration successful\n";
	const char *msg_rtc_report = "Enable time&date reporting(y/n)?: ";


	uint32_t cmd_addr;
	command_t *cmd;
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	while(1){
		/*TODO: Notify wait (wait till someone notifies)		 */
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		/*TODO : Print the menu and show current date and time information */
		xQueueSend(print_queue,(void*)&msg_rtc1, portMAX_DELAY);
		show_time_date();
		xQueueSend(print_queue,(void*)&msg_rtc2, portMAX_DELAY);


		while(curr_state != sMainMenu){

			/*TODO: Wait for command notification (Notify wait) */
			xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);
			cmd = (command_t*)cmd_addr;

			switch(curr_state)
			{
				case sRtcMenu:
					if(cmd->len == 1)
					{
						uint8_t code = cmd->payload[0] - 48;
						switch(code)
						{
							case 0:
								curr_state = sRtcTimeConfig;
								xQueueSend(print_queue,(void*)&msg_rtc_hh, portMAX_DELAY);
								break;
							case 1:
								curr_state = sRtcDateConfig;
								xQueueSend(print_queue,(void*)&msg_rtc_dd, portMAX_DELAY);
								break;
							case 2:
								curr_state = sRtcReport;
								xQueueSend(print_queue,(void*)&msg_rtc_report, portMAX_DELAY);
								break;
							case 3:
								curr_state = sMainMenu;
								break;
							default:
								curr_state = sMainMenu;
								xQueueSend(print_queue, (void*)&msg_inv, portMAX_DELAY);
						}
					} else {
						curr_state = sMainMenu;
						xQueueSend(print_queue, (void*)&msg_inv, portMAX_DELAY);
					}
					break;

				case sRtcTimeConfig:{

					switch(rtc_state){
						case uren:
							time.Hours = (uint8_t)atoi((char*)&cmd->payload);
							rtc_state = minutes;
							xQueueSend(print_queue,(void*)&msg_rtc_mm, portMAX_DELAY);
							break;
						case minutes:
							time.Minutes = (uint8_t)atoi((char*)&cmd->payload);
							rtc_state = secs;
							xQueueSend(print_queue,(void*)&msg_rtc_ss, portMAX_DELAY);
							break;
						case secs:
							time.Seconds = (uint8_t)atoi((char*)&cmd->payload);
							if(!validate_rtc_information(&time, NULL))
							{
								rtc_configure_time(&time);
								xQueueSend(print_queue,(void*)&msg_conf, portMAX_DELAY);
								show_time_date();
							} else xQueueSend(print_queue, (void*)&msg_inv, portMAX_DELAY);

							curr_state = sMainMenu;
							rtc_state = uren;
							break;
					}
				break;}

				case sRtcDateConfig:{

					switch(rtc_date_state){
						case day_config:
							date.Date = (uint8_t)atoi((char*)&cmd->payload);
							rtc_date_state = month_config;
							xQueueSend(print_queue,(void*)&msg_rtc_mo, portMAX_DELAY);
							break;
						case month_config:
							date.Month = (uint8_t)atoi((char*)&cmd->payload);
							rtc_date_state = week_config;
							xQueueSend(print_queue,(void*)&msg_rtc_dow, portMAX_DELAY);
							break;
						case week_config:
							date.WeekDay = (uint8_t)atoi((char*)&cmd->payload);
							rtc_date_state = year_config;
							xQueueSend(print_queue,(void*)&msg_rtc_yr, portMAX_DELAY);
							break;
						case year_config:
							date.Year = (uint8_t)atoi((char*)&cmd->payload);
							if(!validate_rtc_information(NULL, &date))
							{
								rtc_configure_date(&date);
								xQueueSend(print_queue,(void*)&msg_conf, portMAX_DELAY);
								show_time_date();
							} else xQueueSend(print_queue, (void*)&msg_inv, portMAX_DELAY);

							curr_state = sMainMenu;
							rtc_date_state = day_config;
							break;
					}
					break;}

				case sRtcReport:
					/* enable or disable RTC current time reporting over ITM printf */
					if(cmd->len == 1)
					{
						if(cmd->payload[0] == 'y')
							xTimerStart(rtc_timer, portMAX_DELAY);
						else if (cmd->payload[0] == 'n')
							xTimerStop(rtc_timer, portMAX_DELAY);
						else
							xQueueSend(print_queue, (void*)&msg_inv, portMAX_DELAY);
					}
					else
						xQueueSend(print_queue, (void*)&msg_inv, portMAX_DELAY);

				curr_state = sMainMenu;
				break;

				}// switch end

		} //while end

		   /* Notify menu task */
			xTaskNotify(handle_menu_task, 0, eNoAction);

		}//while super loop end
}


#include "uart_i_process.h"
#include "rtx.h"
#include "k_primitives.h"
#include "trace.h"
#include "k_globals.h"
#include "proc_pq.h"
#include "k_hotkeys.h"
#include "user_processes.h"

#define INPUT_BUFFER_SIZE 100

CHAR OutBuffer[IPC_MSG_ENV_MSG_SIZE] = {0};
CHAR InBuffer[INPUT_BUFFER_SIZE] = {0};
uint32_t inputIndex;
uint32_t outputIndex;
bool output_print_char;

#ifdef _CFSERVER_
#define KB_LINE_END '\0'
#else
#define KB_LINE_END '\r'
#endif

/*
 * This function is called by the assembly STUB function
 */
void uart_i_process()
{
    int i;
    BYTE temp;
    temp = SERIAL1_UCSR;    // Ack the interrupt
    volatile BYTE CharIn = ' ';
        
    // There is data to be read
    if( temp & 1 )
    {
        CharIn = SERIAL1_RD;
        if (pong_mode)
        {
            MsgEnv* message = k_request_msg_env();
            message->msg_type = CONSOLE_INPUT;
            message->msg[0] = CharIn;
            k_send_message(PONG_PID, message);
            return;
        }
        if (CharIn == KB_LINE_END)
        {
            SERIAL1_IMR = 3;
            SERIAL1_WD = '\n';
            SERIAL1_IMR = 2;
#ifdef _CFSERVER_
            inputIndex--;
#endif
            InBuffer[inputIndex] = '\0';
            inputIndex++;
            MsgEnv* message = k_request_msg_env();
            if (message != NULL)
            {
                for (i = 0; i < inputIndex; i++)
                {
                    message->msg[i] = InBuffer[i];
                }
                message->msg_type = CONSOLE_INPUT;
                k_send_message(CCI_PID, message);
            }
            inputIndex = 0;
        }
        else if (inputIndex < INPUT_BUFFER_SIZE - 2) // enter in a character
        {
            if (!hotkey(CharIn))
            {
                InBuffer[inputIndex] = CharIn;
                inputIndex++;
                SERIAL1_IMR = 3;
                SERIAL1_WD = CharIn;
                SERIAL1_IMR = 2;
            }
        }
    }
    // Check to see if data can be written out
    else if ( temp & 4 )
    {
        if (outputIndex == 0 && output_print_char == FALSE)
        {
            MsgEnv* message = k_receive_message();
            if (message != NULL)
            {
                i = 0;
                while (message->msg[i] != '\0')
                {
                    OutBuffer[i] = message->msg[i];
                    i++;
                }
                OutBuffer[i] = '\0';
                output_print_char = TRUE;
                if (message->msg_type == CONSOLE_OUTPUT)
                {
                    k_release_msg_env(message);
                }
                else
                {
                    message->msg_type = DISPLAY_ACK;
                    k_send_message(message->send_pid, message);
                }
            }
            else
            {
                trace(ERROR, "Uart i process expected an env but received NULL");
            }
        }
        if (output_print_char)
        {
            if (OutBuffer[outputIndex] == '\0')
            {
                outputIndex = 0;
                output_print_char = FALSE;
                MsgEnv* message = k_receive_message();
                if (message != NULL)
                {
                    i = 0;
                    while (message->msg[i] != '\0')
                    {
                        OutBuffer[i] = message->msg[i];
                        i++;
                    }
                    OutBuffer[i] = '\0';
                    output_print_char = TRUE;
                    if (message->msg_type == CONSOLE_OUTPUT)
                    {
                        k_release_msg_env(message);
                    }
                    else
                    {
                        message->msg_type = DISPLAY_ACK;
                        k_send_message(message->send_pid, message);
                    }
                }
                else
                {
                    SERIAL1_IMR = 2; // Disable tx interrupt
                }
            }
            else
            {
                SERIAL1_WD = OutBuffer[outputIndex]; // Write data
                outputIndex++;
            }
        }
    }
    return;
}

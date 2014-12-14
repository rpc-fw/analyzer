#include "freertos.h"
#include "task.h"

#include "lpc43xx_i2c.h"
#include "fronti2c.h"

extern "C" void M0_I2C0_OR_I2C1_IRQHandler()
{
	I2C_MasterHandler(LPC_I2C0);
}

void FrontI2C::Init()
{
	I2C_Init(LPC_I2C0, 500000);
	I2C_Cmd(LPC_I2C0, ENABLE);
};

bool FrontI2C::Read(uint8_t address, uint8_t* buffer, int count)
{
	//taskENTER_CRITICAL();

	I2C_M_SETUP_Type setup;
	Status status;

	setup.sl_addr7bit = address;
	setup.tx_data     = 0;
	setup.tx_length   = 0;
	setup.rx_data     = buffer;
	setup.rx_length   = count;
	setup.retransmissions_max = 10;

	status = I2C_MasterTransferData(LPC_I2C0, &setup, I2C_TRANSFER_INTERRUPT);
	/*while (!I2C_MasterTransferComplete(LPC_I2C0)) {
		taskYIELD();
	}*/
    bool result = (status == SUCCESS);

    //taskEXIT_CRITICAL();
	return result;
}

bool FrontI2C::Write(uint8_t address, uint8_t* buffer, int count)
{
	//taskENTER_CRITICAL();

    I2C_M_SETUP_Type setup;
    Status status;

    setup.sl_addr7bit = address;
    setup.tx_data     = buffer;
    setup.tx_length   = count;
    setup.rx_data     = 0;
    setup.rx_length   = 0;
    setup.retransmissions_max = 10;

    status = I2C_MasterTransferData(LPC_I2C0, &setup, I2C_TRANSFER_INTERRUPT);
	/*while (!I2C_MasterTransferComplete(LPC_I2C0)) {
		taskYIELD();
	}*/
    bool result = (status == SUCCESS);

    //taskEXIT_CRITICAL();
	return result;
}

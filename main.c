#include "main.h"
#include "keypad.h"
#include "ssd1306.h"
#include "servo.h"
#include "eeprom.h"
#include <string.h>

/* Private variables */
char entered_password[5] = "";   // mat khau nguoi dung
char stored_password[5] = "";    // mat khau duoc luu trong eeprom
int attempt_count = 0;           // so lan nhap sai

int main(void) {
    /* HAL initialization */
    HAL_Init();

    /* System clock configuration */
    SystemClock_Config();

    /* Peripheral initialization */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM1_Init();

    /* Initialize peripherals */
    keypad_Init();
    ssd1306_Init();
    servo_Init(&htim1);

    /* Initialize EEPROM and check stored password */
    eeprom_ReadPassword(stored_password);

    if (stored_password[0] == 0xFF) { // Assuming uninitialized EEPROM
        strcpy(stored_password, "1234"); // pass mac dinh
        eeprom_WritePassword(stored_password);
    }

    /* giao dien mac dinh */
    ssd1306_Fill(SSD1306_COLOR_BLACK);
    ssd1306_WriteString("Enter Password:", Font_11x18, SSD1306_COLOR_WHITE);
    ssd1306_UpdateScreen();

    while (1) {
        /* Wait for user input */
        unlock_door();
    }
}
void unlock_door(void) {
    ssd1306_Fill(SSD1306_COLOR_BLACK);
    ssd1306_WriteString("Enter Password:", Font_11x18, SSD1306_COLOR_WHITE);
    ssd1306_UpdateScreen();

    /* Clear entered password */
    memset(entered_password, 0, sizeof(entered_password));

    /* Wait for password entry */
    for (int i = 0; i < 4; i++) {
        char key = keypad_get_key();
        entered_password[i] = key;

        /* Display asterisks on the OLED */
        ssd1306_WriteChar('*', Font_11x18, SSD1306_COLOR_WHITE);
        ssd1306_UpdateScreen();
    }

    /* Check the entered password */
    if (strcmp(entered_password, stored_password) == 0) {
        /* Correct password */
        ssd1306_Fill(SSD1306_COLOR_BLACK);
        ssd1306_WriteString("Correct!", Font_11x18, SSD1306_COLOR_WHITE);
        ssd1306_UpdateScreen();
        
        /* Rotate the servo to unlock position */
        servo_Write(90);
        HAL_Delay(5000);  // Wait 5 seconds
        
        /* Rotate the servo back to locked position */
        servo_Write(0);
        
        attempt_count = 0;  // Reset attempt counter
    } else {
        /* Incorrect password */
        attempt_count++;
        if (attempt_count >= 3) {
            /* Lockout mechanism */
            ssd1306_Fill(SSD1306_COLOR_BLACK);
            ssd1306_WriteString("Locked! Wait 30s", Font_11x18, SSD1306_COLOR_WHITE);
            ssd1306_UpdateScreen();
            HAL_Delay(30000);  // Wait for 30 seconds
            attempt_count = 0;  // Reset attempt counter
        } else {
            ssd1306_Fill(SSD1306_COLOR_BLACK);
            ssd1306_WriteString("Incorrect!", Font_11x18, SSD1306_COLOR_WHITE);
            ssd1306_UpdateScreen();
            HAL_Delay(2000);
        }
    }

    /* Return to menu */
    ssd1306_Fill(SSD1306_COLOR_BLACK);
    ssd1306_WriteString("1. Unlock Door", Font_11x18, SSD1306_COLOR_WHITE);
    ssd1306_WriteString("2. Change Pwd", Font_11x18, SSD1306_COLOR_WHITE);
    ssd1306_UpdateScreen();
}
void change_password(void) {
    ssd1306_Fill(SSD1306_COLOR_BLACK);
    ssd1306_WriteString("Old Password:", Font_11x18, SSD1306_COLOR_WHITE);
    ssd1306_UpdateScreen();

    /* Clear entered password */
    memset(entered_password, 0, sizeof(entered_password));

    /* Wait for old password entry */
    for (int i = 0; i < 4; i++) {
        char key = keypad_get_key();
        entered_password[i] = key;

        /* Display asterisks on the OLED */
        ssd1306_WriteChar('*', Font_11x18, SSD1306_COLOR_WHITE);
        ssd1306_UpdateScreen();
    }

    /* Check the old password */
    if (strcmp(entered_password, stored_password) == 0) {
        /* Correct old password, prompt for new password */
        ssd1306_Fill(SSD1306_COLOR_BLACK);
        ssd1306_WriteString("New Password:", Font_11x18, SSD1306_COLOR_WHITE);
        ssd1306_UpdateScreen();

        /* Clear entered password */
        memset(entered_password, 0, sizeof(entered_password));

        /* Wait for new password entry */
        for (int i = 0; i < 4; i++) {
            char key = keypad_get_key();
            entered_password[i] = key;

            /* Display asterisks on the OLED */
            ssd1306_WriteChar('*', Font_11x18, SSD1306_COLOR_WHITE);
            ssd1306_UpdateScreen();
        }

        /* Write new password to EEPROM */
        strcpy(stored_password, entered_password);
        eeprom_WritePassword(stored_password);

        ssd1306_Fill(SSD1306_COLOR_BLACK);
        ssd1306_WriteString("Password Changed", Font_11x18, SSD1306_COLOR_WHITE);
        ssd1306_UpdateScreen();
        HAL_Delay(2000);
    } else {
        /* Incorrect old password */
        ssd1306_Fill(SSD1306_COLOR_BLACK);
        ssd1306_WriteString("Incorrect Pwd", Font_11x18, SSD1306_COLOR_WHITE);
        ssd1306_UpdateScreen();
        HAL_Delay(2000);
    }

    /* Return to menu */
    ssd1306_Fill(SSD1306_COLOR_BLACK);
    ssd1306_WriteString("1. Unlock Door", Font_11x18, SSD1306_COLOR_WHITE);
    ssd1306_WriteString("2. Change Pwd", Font_11x18, SSD1306_COLOR_WHITE);
    ssd1306_UpdateScreen();
}
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure GPIO pin for the button */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure GPIO pins for Keypad rows and columns */
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
                          GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* EXTI interrupt init */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}
static void MX_TIM1_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 72 - 1;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 20000 - 1;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;  // 1.5ms pulse width (neutral position)
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    HAL_TIM_MspPostInit(&htim1);
}
void EXTI0_IRQHandler(void) {
    /* Check if EXTI line is pending */
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);

        /* Call the change password function */
        change_password();
    }
}

void Error_Handler(void) {
    /* User can add their own error handling code here */
    while (1) {
        // Stay in loop if error occurs
    }
}

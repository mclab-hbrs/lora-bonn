/*******************************************************************************
 * 
 * Copyright (c) 2021 Hendrik Linka, Hochschule Bonn-Rhein-Sieg
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 *******************************************************************************/

void disable_lora();
void lora_send(float latitude, float longitude, float altitude, float speed, int sats_in_use, float dop_h, int hour, int minute, int second, float thousand);

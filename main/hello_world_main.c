/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "zbus.h"
#include <stdint.h>
#include <stdio.h>

#define LOG_I(_fmt, ...) ESP_LOGI("ZBUS", _fmt, ##__VA_ARGS__)

struct acc_msg {
  int x;
  int y;
  int z;
};

ZBUS_CHAN_DEFINE(acc_data_chan,                        /* Name */
                 struct acc_msg,                       /* Message type */
                 NULL,                                 /* Validator */
                 NULL,                                 /* User data */
                 ZBUS_OBSERVERS(foo_lis, bar_sub),     /* observers */
                 ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0) /* Initial value */
);

ZBUS_CHAN_DEFINE(simple_chan,             /* Name */
                 int,                     /* Message type */
                 NULL,                    /* Validator */
                 NULL,                    /* User data */
                 ZBUS_OBSERVERS(foo_lis), /* observers */
                 0                        /* Initial value is 0 */
);

static void listener_callback_example(const struct zbus_channel *chan) {
  const struct acc_msg *acc = zbus_chan_const_msg(chan);

  LOG_I("From listener -> Acc x=%d, y=%d, z=%d", acc->x, acc->y, acc->z);
}

ZBUS_LISTENER_DEFINE(foo_lis, listener_callback_example);

ZBUS_SUBSCRIBER_DEFINE(bar_sub, 16);

static void subscriber_task(void *pvParameters) {
  const struct zbus_channel *chan;
  LOG_I("Subscriber task running");
  while (!zbus_sub_wait(&bar_sub, &chan, portMAX_DELAY)) {
    struct acc_msg acc;

    if (&acc_data_chan == chan) {
      zbus_chan_read(&acc_data_chan, &acc, 5);

      LOG_I("From subscriber -> Acc x=%d, y=%d, z=%d", acc.x, acc.y, acc.z);
    }
  }
  while (1) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

static bool zbus_chan_init(struct zbus_channel *chan) {
  LOG_I("Channel %s initialized", chan->name);
  chan->mutex = xSemaphoreCreateMutex();
  return true;
}

static bool zbus_observer_init(struct zbus_observer *observer) {
  LOG_I("Observer %s initialized", observer->name);
  if (observer->callback == NULL) {
    observer->queue = xQueueCreate(1, 16);
  }

  return true;
}

__used int zbus_all_chan_init() {
  zbus_iterate_over_channels(zbus_chan_init);
  return 0;
}

__used int zbus_all_observer_init() {
  zbus_iterate_over_observers(zbus_observer_init);
  return 0;
}

void zbus_init(void) {
  zbus_all_chan_init();
  zbus_all_observer_init();
}

int app_main(void) {

  zbus_init();
  xTaskCreate(subscriber_task, "subscriber_task", 2048, NULL, 10, NULL);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  struct acc_msg acc1 = {.x = 1, .y = 1, .z = 1};
  zbus_chan_pub(&acc_data_chan, &acc1, 10);

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  acc1.x = 2;
  acc1.y = 2;
  acc1.z = 2;
  zbus_chan_pub(&acc_data_chan, &acc1, 10);
  return 0;
}
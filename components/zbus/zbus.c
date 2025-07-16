/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zbus.h"
#include "esp_err.h"
#include "portmacro.h"

BaseType_t k_msgq_put(QueueHandle_t xQueue, const void *const pvItemToQueue,
                      TickType_t xTicksToWait) {
  return !xQueueGenericSend((xQueue), (pvItemToQueue), (xTicksToWait),
                            queueSEND_TO_BACK);
}

BaseType_t k_msgq_get(QueueHandle_t xQueue, void *const pvBuffer,
                      TickType_t xTicksToWait) {
  return !xQueueReceive((xQueue), (pvBuffer), (xTicksToWait));
}

BaseType_t k_mutex_lock(QueueHandle_t xQueue, TickType_t xTicksToWait) {
  return !xQueueSemaphoreTake((xQueue), (xTicksToWait));
}

BaseType_t k_mutex_unlock(QueueHandle_t xSemaphore) {
  return !xQueueGenericSend((QueueHandle_t)(xSemaphore), NULL,
                            semGIVE_BLOCK_TIME, queueSEND_TO_BACK);
}

k_timeout_t _zbus_timeout_remainder(k_timeout_t end_ticks) {
  k_timeout_t now_ticks = sys_clock_tick_get();

  return (k_timeout_t)MAX(end_ticks - now_ticks, 0);
}

k_timeout_t sys_clock_timeout_end_calc(k_timeout_t timeout) {
  k_timeout_t end_tick = 0;

  if (timeout == portMAX_DELAY) {
    end_tick = portMAX_DELAY;
  } else if (timeout == 0) {
    end_tick = sys_clock_tick_get();
  } else {
    end_tick = sys_clock_tick_get() + MAX(1, timeout);
  }

  return end_tick;
}

static int _zbus_notify_observers(const struct zbus_channel *chan,
                                  uint64_t end_ticks) {
  int last_error = 0, err;
  /* Notify static listeners */
  for (const struct zbus_observer *const *obs = chan->observers; *obs != NULL;
       ++obs) {
    if ((*obs)->enabled && ((*obs)->callback != NULL)) {
      (*obs)->callback(chan);
    }
  }

  /* Notify static subscribers */
  for (const struct zbus_observer *const *obs = chan->observers; *obs != NULL;
       ++obs) {
    if ((*obs)->enabled && ((*obs)->queue != NULL)) {
      err =
          k_msgq_put((*obs)->queue, &chan, _zbus_timeout_remainder(end_ticks));
      _ZBUS_ASSERT(err == 0, "could not deliver notification to observer %s.",
                   _ZBUS_OBS_NAME(*obs));
      if (err) {
        LOG_ERR("Observer %s at %p could not be notified. Error code %d",
                _ZBUS_OBS_NAME(*obs), *obs, err);
        last_error = err;
      }
    }
  }

  return last_error;
}

int zbus_chan_pub(const struct zbus_channel *chan, const void *msg,
                  k_timeout_t timeout) {
  int err;
  uint64_t end_ticks = sys_clock_timeout_end_calc(timeout);

  _ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  if (chan->validator != NULL && !chan->validator(msg, chan->message_size)) {
    return -ENOMSG;
  }

  err = k_mutex_lock(chan->mutex, timeout);
  if (err) {
    return err;
  }

  memcpy(chan->message, msg, chan->message_size);

  err = _zbus_notify_observers(chan, end_ticks);

  k_mutex_unlock(chan->mutex);

  return err;
}

int zbus_chan_read(const struct zbus_channel *chan, void *msg,
                   k_timeout_t timeout) {
  int err;

  _ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  err = k_mutex_lock(chan->mutex, timeout);
  if (err) {
    return err;
  }

  memcpy(msg, chan->message, chan->message_size);

  return k_mutex_unlock(chan->mutex);
}

int zbus_chan_notify(const struct zbus_channel *chan, k_timeout_t timeout) {
  int err;
  uint64_t end_ticks = sys_clock_timeout_end_calc(timeout);

  _ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  err = k_mutex_lock(chan->mutex, timeout);
  if (err) {
    return err;
  }

  err = _zbus_notify_observers(chan, end_ticks);

  k_mutex_unlock(chan->mutex);

  return err;
}

int zbus_chan_claim(const struct zbus_channel *chan, k_timeout_t timeout) {
  _ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  int err = k_mutex_lock(chan->mutex, timeout);

  if (err) {
    return err;
  }

  return 0;
}

int zbus_chan_finish(const struct zbus_channel *chan) {
  _ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  int err = k_mutex_unlock(chan->mutex);

  return err;
}

int zbus_sub_wait(const struct zbus_observer *sub,
                  const struct zbus_channel **chan, k_timeout_t timeout) {
  _ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
  _ZBUS_ASSERT(sub != NULL, "sub is required");
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  if (sub->queue == NULL) {
    return -EINVAL;
  }

  return k_msgq_get(sub->queue, chan, timeout);
}

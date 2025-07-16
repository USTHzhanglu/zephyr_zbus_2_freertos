/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zbus.h"

/**
 * @brief Iterate over a specified iterable section (alternate).
 *
 * @details
 * Iterator for structure instances gathered by STRUCT_SECTION_ITERABLE().
 * The linker must provide a _<out_type>_list_start symbol and a
 * _<out_type>_list_end symbol to mark the start and the end of the
 * list of struct objects to iterate over. This is normally done using
 * ITERABLE_SECTION_ROM() or ITERABLE_SECTION_RAM() in the linker script.
 */
#define STRUCT_SECTION_FOREACH_ALTERNATE(out_type, struct_type, iterator)                          \
	extern struct struct_type _CONCAT(_##out_type, _list_start)[];                             \
	extern struct struct_type _CONCAT(_##out_type, _list_end)[];                               \
	for (struct struct_type *iterator = _CONCAT(_##out_type, _list_start); ({                  \
		     __ASSERT(iterator <= _CONCAT(_##out_type, _list_end),                         \
			      "unexpected list end location");                                     \
		     iterator < _CONCAT(_##out_type, _list_end);                                   \
	     });                                                                                   \
	     iterator++)

/**
 * @brief Iterate over a specified iterable section.
 *
 * @details
 * Iterator for structure instances gathered by STRUCT_SECTION_ITERABLE().
 * The linker must provide a _<struct_type>_list_start symbol and a
 * _<struct_type>_list_end symbol to mark the start and the end of the
 * list of struct objects to iterate over. This is normally done using
 * ITERABLE_SECTION_ROM() or ITERABLE_SECTION_RAM() in the linker script.
 */
#define STRUCT_SECTION_FOREACH(struct_type, iterator) \
	STRUCT_SECTION_FOREACH_ALTERNATE(struct_type, struct_type, iterator)

bool zbus_iterate_over_channels(bool (*iterator_func)(struct zbus_channel *chan))
{
	STRUCT_SECTION_FOREACH(zbus_channel, chan) {
		if (!(*iterator_func)(chan)) {
			return false;
		}
	}
	return true;
}

bool zbus_iterate_over_observers(bool (*iterator_func)(struct zbus_observer *obs))
{
	STRUCT_SECTION_FOREACH(zbus_observer, obs) {
		if (!(*iterator_func)(obs)) {
			return false;
		}
	}
	return true;
}

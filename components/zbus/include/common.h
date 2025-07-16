/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_TOOLCHAIN_COMMON_H_
#define INCLUDE_TOOLCHAIN_COMMON_H_
/**
 * @file
 * @brief Common toolchain abstraction
 *
 * Macros to abstract compiler capabilities (common to all toolchains).
 */
#define __noasan __attribute__((no_sanitize("address")))
#define Z_STRINGIFY(x) #x
#define STRINGIFY(s) Z_STRINGIFY(s)

#define ___in_section(a, b, c) \
	__attribute__((section("." Z_STRINGIFY(a)			\
				"." Z_STRINGIFY(b)			\
				"." Z_STRINGIFY(c))))
#define __in_section(a, b, c) ___in_section(a, b, c)

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

/*
 * This is meant to be used in conjunction with __in_section() and similar
 * where scattered structure instances are concatenated together by the linker
 * and walked by the code at run time just like a contiguous array of such
 * structures.
 *
 * Assemblers and linkers may insert alignment padding by default whose
 * size is larger than the natural alignment for those structures when
 * gathering various section segments together, messing up the array walk.
 * To prevent this, we need to provide an explicit alignment not to rely
 * on the default that might just work by luck.
 *
 * Alignment statements in  linker scripts are not sufficient as
 * the assembler may add padding by itself to each segment when switching
 * between sections within the same file even if it merges many such segments
 * into a single section in the end.
 */
#define Z_DECL_ALIGN(type) __aligned(__alignof(type)) type

/**
 * @brief Defines a new element for an iterable section.
 *
 * @details
 * Convenience helper combining __in_section() and Z_DECL_ALIGN().
 * The section name is the struct type prepended with an underscore.
 * The subsection is "static" and the subsubsection is the variable name.
 *
 * In the linker script, create output sections for these using
 * ITERABLE_SECTION_ROM() or ITERABLE_SECTION_RAM().
 *
 * @note In order to store the element in ROM, a const specifier has to
 * be added to the declaration: const STRUCT_SECTION_ITERABLE(...);
 */
#define STRUCT_SECTION_ITERABLE(struct_type, name) \
	struct struct_type name \
	__in_section(_##struct_type, static, name) __used



#endif /* INCLUDE_TOOLCHAIN_COMMON_H_ */

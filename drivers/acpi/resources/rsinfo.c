/*******************************************************************************
 *
 * Module Name: rsinfo - Dispatch and Info tables
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2005, R. Byron Moore
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <acpi/acpi.h>
#include <acpi/acresrc.h>

#define _COMPONENT          ACPI_RESOURCES
ACPI_MODULE_NAME("rsinfo")

/*
 * Resource dispatch and information tables. Any new resource types (either
 * Large or Small) must be reflected in each of these tables, so they are here
 * in one place.
 *
 * The tables for Large descriptors are indexed by bits 6:0 of the AML
 * descriptor type byte. The tables for Small descriptors are indexed by
 * bits 6:3 of the descriptor byte. The tables for internal resource
 * descriptors are indexed by the acpi_resource_type field.
 */
/* Dispatch table for resource-to-AML (Set Resource) conversion functions */
ACPI_SET_RESOURCE_HANDLER acpi_gbl_set_resource_dispatch[] = {
	acpi_rs_set_irq,	/* 0x00, ACPI_RESOURCE_TYPE_IRQ */
	acpi_rs_set_dma,	/* 0x01, ACPI_RESOURCE_TYPE_DMA */
	acpi_rs_set_start_dpf,	/* 0x02, ACPI_RESOURCE_TYPE_START_DEPENDENT */
	acpi_rs_set_end_dpf,	/* 0x03, ACPI_RESOURCE_TYPE_END_DEPENDENT */
	acpi_rs_set_io,		/* 0x04, ACPI_RESOURCE_TYPE_IO */
	acpi_rs_set_fixed_io,	/* 0x05, ACPI_RESOURCE_TYPE_FIXED_IO */
	acpi_rs_set_vendor,	/* 0x06, ACPI_RESOURCE_TYPE_VENDOR */
	acpi_rs_set_end_tag,	/* 0x07, ACPI_RESOURCE_TYPE_END_TAG */
	acpi_rs_set_memory24,	/* 0x08, ACPI_RESOURCE_TYPE_MEMORY24 */
	acpi_rs_set_memory32,	/* 0x09, ACPI_RESOURCE_TYPE_MEMORY32 */
	acpi_rs_set_fixed_memory32,	/* 0x0A, ACPI_RESOURCE_TYPE_FIXED_MEMORY32 */
	acpi_rs_set_address16,	/* 0x0B, ACPI_RESOURCE_TYPE_ADDRESS16 */
	acpi_rs_set_address32,	/* 0x0C, ACPI_RESOURCE_TYPE_ADDRESS32 */
	acpi_rs_set_address64,	/* 0x0D, ACPI_RESOURCE_TYPE_ADDRESS64 */
	acpi_rs_set_ext_address64,	/* 0x0E, ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64 */
	acpi_rs_set_ext_irq,	/* 0x0F, ACPI_RESOURCE_TYPE_EXTENDED_IRQ */
	acpi_rs_set_generic_reg	/* 0x10, ACPI_RESOURCE_TYPE_GENERIC_REGISTER */
};

/* Dispatch tables for AML-to-resource (Get Resource) conversion functions */

ACPI_GET_RESOURCE_HANDLER acpi_gbl_sm_get_resource_dispatch[] = {
	NULL,			/* 0x00, Reserved */
	NULL,			/* 0x01, Reserved */
	NULL,			/* 0x02, Reserved */
	NULL,			/* 0x03, Reserved */
	acpi_rs_get_irq,	/* 0x04, ACPI_RESOURCE_NAME_IRQ */
	acpi_rs_get_dma,	/* 0x05, ACPI_RESOURCE_NAME_DMA */
	acpi_rs_get_start_dpf,	/* 0x06, ACPI_RESOURCE_NAME_START_DEPENDENT */
	acpi_rs_get_end_dpf,	/* 0x07, ACPI_RESOURCE_NAME_END_DEPENDENT */
	acpi_rs_get_io,		/* 0x08, ACPI_RESOURCE_NAME_IO */
	acpi_rs_get_fixed_io,	/* 0x09, ACPI_RESOURCE_NAME_FIXED_IO */
	NULL,			/* 0x0A, Reserved */
	NULL,			/* 0x0B, Reserved */
	NULL,			/* 0x0C, Reserved */
	NULL,			/* 0x0D, Reserved */
	acpi_rs_get_vendor,	/* 0x0E, ACPI_RESOURCE_NAME_VENDOR_SMALL */
	acpi_rs_get_end_tag	/* 0x0F, ACPI_RESOURCE_NAME_END_TAG */
};

ACPI_GET_RESOURCE_HANDLER acpi_gbl_lg_get_resource_dispatch[] = {
	NULL,			/* 0x00, Reserved */
	acpi_rs_get_memory24,	/* 0x01, ACPI_RESOURCE_NAME_MEMORY24 */
	acpi_rs_get_generic_reg,	/* 0x02, ACPI_RESOURCE_NAME_GENERIC_REGISTER */
	NULL,			/* 0x03, Reserved */
	acpi_rs_get_vendor,	/* 0x04, ACPI_RESOURCE_NAME_VENDOR_LARGE */
	acpi_rs_get_memory32,	/* 0x05, ACPI_RESOURCE_NAME_MEMORY32 */
	acpi_rs_get_fixed_memory32,	/* 0x06, ACPI_RESOURCE_NAME_FIXED_MEMORY32 */
	acpi_rs_get_address32,	/* 0x07, ACPI_RESOURCE_NAME_ADDRESS32 */
	acpi_rs_get_address16,	/* 0x08, ACPI_RESOURCE_NAME_ADDRESS16 */
	acpi_rs_get_ext_irq,	/* 0x09, ACPI_RESOURCE_NAME_EXTENDED_IRQ */
	acpi_rs_get_address64,	/* 0x0A, ACPI_RESOURCE_NAME_ADDRESS64 */
	acpi_rs_get_ext_address64	/* 0x0B, ACPI_RESOURCE_NAME_EXTENDED_ADDRESS64 */
};

#ifdef ACPI_FUTURE_USAGE
#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DEBUGGER)

/* Dispatch table for resource dump functions */

ACPI_DUMP_RESOURCE_HANDLER acpi_gbl_dump_resource_dispatch[] = {
	acpi_rs_dump_irq,	/* ACPI_RESOURCE_TYPE_IRQ */
	acpi_rs_dump_dma,	/* ACPI_RESOURCE_TYPE_DMA */
	acpi_rs_dump_start_dpf,	/* ACPI_RESOURCE_TYPE_START_DEPENDENT */
	acpi_rs_dump_end_dpf,	/* ACPI_RESOURCE_TYPE_END_DEPENDENT */
	acpi_rs_dump_io,	/* ACPI_RESOURCE_TYPE_IO */
	acpi_rs_dump_fixed_io,	/* ACPI_RESOURCE_TYPE_FIXED_IO */
	acpi_rs_dump_vendor,	/* ACPI_RESOURCE_TYPE_VENDOR */
	acpi_rs_dump_end_tag,	/* ACPI_RESOURCE_TYPE_END_TAG */
	acpi_rs_dump_memory24,	/* ACPI_RESOURCE_TYPE_MEMORY24 */
	acpi_rs_dump_memory32,	/* ACPI_RESOURCE_TYPE_MEMORY32 */
	acpi_rs_dump_fixed_memory32,	/* ACPI_RESOURCE_TYPE_FIXED_MEMORY32 */
	acpi_rs_dump_address16,	/* ACPI_RESOURCE_TYPE_ADDRESS16 */
	acpi_rs_dump_address32,	/* ACPI_RESOURCE_TYPE_ADDRESS32 */
	acpi_rs_dump_address64,	/* ACPI_RESOURCE_TYPE_ADDRESS64 */
	acpi_rs_dump_ext_address64,	/* ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64 */
	acpi_rs_dump_ext_irq,	/* ACPI_RESOURCE_TYPE_EXTENDED_IRQ */
	acpi_rs_dump_generic_reg	/* ACPI_RESOURCE_TYPE_GENERIC_REGISTER */
};
#endif
#endif	/* ACPI_FUTURE_USAGE */

/*
 * Base sizes for external AML resource descriptors, indexed by internal type.
 * Includes size of the descriptor header (1 byte for small descriptors,
 * 3 bytes for large descriptors)
 */
u8 acpi_gbl_aml_resource_sizes[] = {
	sizeof(struct aml_resource_irq),	/* ACPI_RESOURCE_TYPE_IRQ (optional Byte 3 always created) */
	sizeof(struct aml_resource_dma),	/* ACPI_RESOURCE_TYPE_DMA */
	sizeof(struct aml_resource_start_dependent),	/* ACPI_RESOURCE_TYPE_START_DEPENDENT (optional Byte 1 always created) */
	sizeof(struct aml_resource_end_dependent),	/* ACPI_RESOURCE_TYPE_END_DEPENDENT */
	sizeof(struct aml_resource_io),	/* ACPI_RESOURCE_TYPE_IO */
	sizeof(struct aml_resource_fixed_io),	/* ACPI_RESOURCE_TYPE_FIXED_IO */
	sizeof(struct aml_resource_vendor_small),	/* ACPI_RESOURCE_TYPE_VENDOR */
	sizeof(struct aml_resource_end_tag),	/* ACPI_RESOURCE_TYPE_END_TAG */
	sizeof(struct aml_resource_memory24),	/* ACPI_RESOURCE_TYPE_MEMORY24 */
	sizeof(struct aml_resource_memory32),	/* ACPI_RESOURCE_TYPE_MEMORY32 */
	sizeof(struct aml_resource_fixed_memory32),	/* ACPI_RESOURCE_TYPE_FIXED_MEMORY32 */
	sizeof(struct aml_resource_address16),	/* ACPI_RESOURCE_TYPE_ADDRESS16 */
	sizeof(struct aml_resource_address32),	/* ACPI_RESOURCE_TYPE_ADDRESS32 */
	sizeof(struct aml_resource_address64),	/* ACPI_RESOURCE_TYPE_ADDRESS64 */
	sizeof(struct aml_resource_extended_address64),	/*ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64 */
	sizeof(struct aml_resource_extended_irq),	/* ACPI_RESOURCE_TYPE_EXTENDED_IRQ */
	sizeof(struct aml_resource_generic_register)	/* ACPI_RESOURCE_TYPE_GENERIC_REGISTER */
};

/* Macros used in the tables below */

#define ACPI_RLARGE(r)          sizeof (r) - sizeof (struct aml_resource_large_header)
#define ACPI_RSMALL(r)          sizeof (r) - sizeof (struct aml_resource_small_header)

/*
 * Base sizes of resource descriptors, both the AML stream resource length
 * (minus size of header and length fields),and the size of the internal
 * struct representation.
 */
struct acpi_resource_info acpi_gbl_sm_resource_info[] = {
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{2, ACPI_RSMALL(struct aml_resource_irq),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_irq)},
	{0, ACPI_RSMALL(struct aml_resource_dma),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_dma)},
	{2, ACPI_RSMALL(struct aml_resource_start_dependent),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_start_dependent)},
	{0, ACPI_RSMALL(struct aml_resource_end_dependent),
	 ACPI_RESOURCE_LENGTH},
	{0, ACPI_RSMALL(struct aml_resource_io),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_io)},
	{0, ACPI_RSMALL(struct aml_resource_fixed_io),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_fixed_io)},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{1, ACPI_RSMALL(struct aml_resource_vendor_small),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_vendor)},
	{0, ACPI_RSMALL(struct aml_resource_end_tag), ACPI_RESOURCE_LENGTH}
};

struct acpi_resource_info acpi_gbl_lg_resource_info[] = {
	{0, 0, 0},
	{0, ACPI_RLARGE(struct aml_resource_memory24),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_memory24)},
	{0, ACPI_RLARGE(struct aml_resource_generic_register),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_generic_registerister)},
	{0, 0, 0},
	{1, ACPI_RLARGE(struct aml_resource_vendor_large),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_vendor)},
	{0, ACPI_RLARGE(struct aml_resource_memory32),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_memory32)},
	{0, ACPI_RLARGE(struct aml_resource_fixed_memory32),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_fixed_memory32)},
	{1, ACPI_RLARGE(struct aml_resource_address32),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_address32)},
	{1, ACPI_RLARGE(struct aml_resource_address16),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_address16)},
	{1, ACPI_RLARGE(struct aml_resource_extended_irq),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_extended_irq)},
	{1, ACPI_RLARGE(struct aml_resource_address64),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_address64)},
	{0, ACPI_RLARGE(struct aml_resource_extended_address64),
	 ACPI_SIZEOF_RESOURCE(struct acpi_resource_extended_address64)}
};

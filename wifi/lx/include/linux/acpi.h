/*
 * linux/acpi.h for ath10k's mac.c, which asks ACPI for a regulatory domain on a PC (ath10k_mac_get_wrdd_regulatory()):
 * a phone has no ACPI. ACPI_HANDLE() of a device is nothing, which is where the driver stops asking; the rest is here
 * for that function to compile. (FreeBSD's own header takes ACPICA's, which has no part for this compiler's target.)
 */
#ifndef LX_ACPI_H
#define LX_ACPI_H

#include <linux/types.h>

typedef void *acpi_handle;
typedef u32 acpi_status;
typedef char *acpi_string;
typedef u32 acpi_object_type;

#define ACPI_TYPE_INTEGER       0x01
#define ACPI_TYPE_PACKAGE       0x04
#define ACPI_ALLOCATE_BUFFER    ((size_t)-1)
#define AE_OK                   0
#define AE_NOT_FOUND            5
#define ACPI_FAILURE(status)    ((status) != AE_OK)
#define ACPI_SUCCESS(status)    ((status) == AE_OK)
#define ACPI_HANDLE(dev)        ((acpi_handle)NULL)
#define ACPI_COMPANION(dev)     NULL

union acpi_object {
	acpi_object_type type;
	struct { acpi_object_type type; u64 value; } integer;
	struct { acpi_object_type type; u32 count; union acpi_object *elements; } package;
};

struct acpi_buffer {
	size_t length;
	void *pointer;
};

struct acpi_object_list {
	u32 count;
	union acpi_object *pointer;
};

static inline acpi_status
acpi_get_handle(acpi_handle parent, acpi_string path, acpi_handle *out)
{
	(void)parent; (void)path; (void)out;
	return AE_NOT_FOUND;
}

static inline acpi_status
acpi_evaluate_object(acpi_handle object, acpi_string path, struct acpi_object_list *arguments, struct acpi_buffer *out)
{
	(void)object; (void)path; (void)arguments; (void)out;
	return AE_NOT_FOUND;
}

#endif

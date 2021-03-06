// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019, Linaro Limited
 */

#include <common.h>
#include <dm.h>
#include <efi_loader.h>
#include <efi_rng.h>
#include <rng.h>

DECLARE_GLOBAL_DATA_PTR;

const efi_guid_t efi_guid_rng_protocol = EFI_RNG_PROTOCOL_GUID;

__weak efi_status_t platform_get_rng_device(struct udevice **dev)
{
	int ret;
	struct udevice *devp;

	ret = uclass_get_device(UCLASS_RNG, 0, &devp);
	if (ret) {
		debug("Unable to get rng device\n");
		return EFI_DEVICE_ERROR;
	}

	*dev = devp;

	return EFI_SUCCESS;
}

static efi_status_t EFIAPI rng_getinfo(struct efi_rng_protocol *this,
				       efi_uintn_t *rng_algorithm_list_size,
				       efi_guid_t *rng_algorithm_list)
{
	efi_status_t ret = EFI_SUCCESS;
	efi_guid_t rng_algo_guid = EFI_RNG_ALGORITHM_RAW;

	EFI_ENTRY("%p, %p, %p", this, rng_algorithm_list_size,
		  rng_algorithm_list);

	if (!this || !rng_algorithm_list_size) {
		ret = EFI_INVALID_PARAMETER;
		goto back;
	}

	if (!rng_algorithm_list ||
	    *rng_algorithm_list_size < sizeof(*rng_algorithm_list)) {
		*rng_algorithm_list_size = sizeof(*rng_algorithm_list);
		ret = EFI_BUFFER_TOO_SMALL;
		goto back;
	}

	/*
	 * For now, use EFI_RNG_ALGORITHM_RAW as the default
	 * algorithm. If a new algorithm gets added in the
	 * future through a Kconfig, rng_algo_guid will be set
	 * based on that Kconfig option
	 */
	*rng_algorithm_list_size = sizeof(*rng_algorithm_list);
	guidcpy(rng_algorithm_list, &rng_algo_guid);

back:
	return EFI_EXIT(ret);
}

static efi_status_t EFIAPI getrng(struct efi_rng_protocol *this,
				  efi_guid_t *rng_algorithm,
				  efi_uintn_t rng_value_length,
				  uint8_t *rng_value)
{
	int ret;
	efi_status_t status = EFI_SUCCESS;
	struct udevice *dev;
	const efi_guid_t rng_raw_guid = EFI_RNG_ALGORITHM_RAW;

	EFI_ENTRY("%p, %p, %zu, %p", this, rng_algorithm, rng_value_length,
		  rng_value);

	if (!this || !rng_value || !rng_value_length) {
		status = EFI_INVALID_PARAMETER;
		goto back;
	}

	if (rng_algorithm) {
		EFI_PRINT("RNG algorithm %pUl\n", rng_algorithm);
		if (guidcmp(rng_algorithm, &rng_raw_guid)) {
			status = EFI_UNSUPPORTED;
			goto back;
		}
	}

	ret = platform_get_rng_device(&dev);
	if (ret != EFI_SUCCESS) {
		EFI_PRINT("Rng device not found\n");
		status = EFI_UNSUPPORTED;
		goto back;
	}

	ret = dm_rng_read(dev, rng_value, rng_value_length);
	if (ret < 0) {
		EFI_PRINT("Rng device read failed\n");
		status = EFI_DEVICE_ERROR;
		goto back;
	}

back:
	return EFI_EXIT(status);
}

const struct efi_rng_protocol efi_rng_protocol = {
	.get_info = rng_getinfo,
	.get_rng = getrng,
};

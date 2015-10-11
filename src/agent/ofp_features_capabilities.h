/* %COPYRIGHT% */

/**
 * @file	ofp_features_capabilities.h
 */

#ifndef __OFP_FEATURES_CAPABILITIES_H__
#define __OFP_FEATURES_CAPABILITIES_H__

/**
 * Convert datastore capabilities => features capabilities.
 *
 *     @param[in]	ds_capabilities	capabilities (format to datastore capabilities).
 *     @param[in,out]	features_capabilities	A pointer to features.
 *                                              (format to features capabilities)
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_features_capabilities_convert(uint64_t ds_capabilities,
                                  uint32_t *features_capabilities);

#endif /* __OFP_FEATURES_CAPABILITIES_H__ */

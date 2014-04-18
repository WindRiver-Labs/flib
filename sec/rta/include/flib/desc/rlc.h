/* Copyright 2014 Freescale Semiconductor, Inc. */

#ifndef __DESC_RLC_H__
#define __DESC_RLC_H__

#include "flib/rta.h"
#include "common.h"
#include "pdcp.h"

/**
 * @file                 rlc.h
 * @brief                SEC Descriptor Construction Library Protocol-level
 *                       WCDMA RLC Shared Descriptor Constructors
 */

/**
 * @defgroup descriptor_lib_group RTA Descriptors Library
 * @{
 */
/** @} end of descriptor_lib_group */

/*
 * RLC Protocol Data Blocks
 */
#define RLC_PDB_OPT_SNS_SHIFT	1
#define RLC_PDB_OPT_SNS_AM	(0 << RLC_PDB_OPT_SNS_SHIFT)
#define RLC_PDB_OPT_SNS_UM	(1 << RLC_PDB_OPT_SNS_SHIFT)

#define RLC_PDB_OPT_4B_SHIFT	0
#define RLC_PDB_OPT_4B_SHIFT_EN	(1 << RLC_PDB_OPT_4B_SHIFT)

#define RLC_PDB_HFN_SHIFT_UM	7
#define RLC_PDB_HFN_SHIFT_AM	12

#define RLC_PDB_BEARER_SHIFT	27
#define RLC_PDB_DIR_SHIFT	26

/**
 * @defgroup typedefs_group Auxiliary Data Structures
 * @ingroup descriptor_lib_group
 * @{
 */

/**
 * @enum     rlc_mode protoshared.h
 * @details  WCDMA RLC mode selector
 */
enum rlc_mode {
	RLC_UNACKED_MODE = 7,
	RLC_ACKED_MODE = 12
};

/**
 * @enum     rlc_dir protoshared.h
 * @details  WCDMA RLC direction selector
 */
enum rlc_dir {
	RLC_DIR_UPLINK,
	RLC_DIR_DOWNLINK
};

/**
 * @enum      cipher_type_rlc protoshared.h
 * @details   Type selectors for cipher types in RLC protocol OP instructions.
 */
enum cipher_type_rlc {
	RLC_CIPHER_TYPE_NULL,
	RLC_CIPHER_TYPE_KASUMI,
	RLC_CIPHER_TYPE_SNOW,
	RLC_CIPHER_TYPE_INVALID
};

/** @} */ /* end of typedefs_group */

struct rlc_pdb {
	uint32_t opt_res;	/* RLC options bitfield:
				 * - bit 30: 1 = unacknowledged mode
				 *           0 = acknowldeged mode
				 * - bit 31: 1 = shift output data
				 *               by 4 bits (pad with 0)
				 */
	uint32_t hfn_res;	/* HyperFrame number,(27 or 20 bits), left
				   aligned & right-padded with zeros. */
	uint32_t bearer_dir_res;/* Bearer(5 bits), packet direction (1 bit),
				 * left aligned & right-padded with zeros. */
	uint32_t hfn_thr_res;	/* HyperFrame number threshold (27 or 20 bits),
				   left aligned & right-padded with zeros. */
};
/*
 * RLC internal PDB type
 */
#define RLC_PDB_TYPE_FULL_PDB PDCP_PDB_TYPE_FULL_PDB

/**
 * @defgroup sharedesc_group Shared Descriptor Example Routines
 * @ingroup descriptor_lib_group
 * @{
 */
/** @} end of sharedesc_group */

/**
 * @details                  Function for creating a WCDMA RLC
 *                           encapsulation descriptor.
 * @ingroup                  sharedesc_group
 *
 * @param[in,out] descbuf    Pointer to buffer for descriptor construction.
 * @param[in,out] bufsize    Size of descriptor written. Once the function
 *                           returns, the value of this parameter can be used
 *                           for reclaiming the space that wasn't used for the
 *                           descriptor.
 * @param[in] ps             If 36/40bit addressing is desired, this parameter
 *                           must be non-zero.
 * @param[in] mode           Indicates if ACKed or non-ACKed mode is used
 * @param[in] hfn            Starting Hyper Frame Number to be used together
 *                           with the SN from the RLC frames.
 * @param[in] bearer         Radio bearer ID.
 * @param[in] direction      The direction of the RLC PDU (UL/DL).
 * @param[in] hfn_threshold  HFN value that once reached triggers a warning
 *                           from SEC that keys should be renegociated at the
 *                           earliest convenience.
 * @param[in] cipherdata     Pointer to block cipher transform definitions.
 *                           Valid algorithm values are those from
 *                           cipher_type_rlc enum.
 *
 * @note  @b descbuf must be large enough to contain a full 256 byte long
 *        descriptor; after the function returns, by subtracting the actual
 *        number of bytes used (using @b bufsize), the user can reuse the
 *        remaining buffer space for other purposes.
 *
 */
static inline void cnstr_shdsc_rlc_encap(uint32_t *descbuf,
		unsigned *bufsize,
		unsigned short ps,
		enum rlc_mode mode,
		uint32_t hfn,
		unsigned short bearer,
		unsigned short direction,
		uint32_t hfn_threshold,
		struct alginfo *cipherdata)
{
	struct program prg;
	struct program *program = &prg;
	struct rlc_pdb pdb;
	LABEL(pdb_end);
	LABEL(keyjmp);
	REFERENCE(phdr);
	REFERENCE(pkeyjmp);

	PROGRAM_CNTXT_INIT(descbuf, 0);

	if (ps)
		PROGRAM_SET_36BIT_ADDR();

	phdr = SHR_HDR(SHR_SERIAL, 0, WITH(0));

	memset(&pdb, 0, sizeof(struct rlc_pdb));

	/*
	 * Read options from user:  depending on sequence number length,
	 * the HFN and HFN threshold have different lengths.
	 */
	switch (mode) {
	case RLC_UNACKED_MODE:
		pdb.opt_res = RLC_PDB_OPT_SNS_UM;
		pdb.hfn_res = hfn << RLC_PDB_HFN_SHIFT_UM;
		pdb.hfn_thr_res = hfn_threshold << RLC_PDB_HFN_SHIFT_UM;
		break;

	case RLC_ACKED_MODE:
		pdb.opt_res = RLC_PDB_OPT_SNS_AM;
		pdb.hfn_res = hfn << RLC_PDB_HFN_SHIFT_AM;
		pdb.hfn_thr_res = hfn_threshold << RLC_PDB_HFN_SHIFT_AM;
		break;

	default:
		pr_debug("Invalid RLC mode setting in PDB\n");
		return;
	}

	pdb.bearer_dir_res = (uint32_t)
				((bearer << RLC_PDB_BEARER_SHIFT) |
				 (direction << RLC_PDB_DIR_SHIFT));

	/* copy PDB in descriptor*/
	COPY_DATA((uint8_t *)&pdb, sizeof(struct rlc_pdb));

	SET_LABEL(pdb_end);

	if (insert_hfn_ov_op(program, mode, RLC_PDB_TYPE_FULL_PDB, 0))
		return;

	switch (mode) {
	case RLC_UNACKED_MODE:
	case RLC_ACKED_MODE:
		switch (cipherdata->algtype) {
		case RLC_CIPHER_TYPE_KASUMI:
			pkeyjmp = JUMP(IMM(keyjmp), LOCAL_JUMP, ALL_TRUE,
				       WITH(SHRD));
			/* Insert Cipher Key */
			KEY(KEY1, cipherdata->key_enc_flags,
			    PTR(cipherdata->key), cipherdata->keylen, WITH(0));
			SET_LABEL(keyjmp);

			PROTOCOL(OP_TYPE_ENCAP_PROTOCOL,
				 OP_PCLID_3G_RLC_PDU,
				 (uint16_t)cipherdata->algtype);

			PATCH_JUMP(pkeyjmp, keyjmp);

			break;

		case RLC_CIPHER_TYPE_SNOW:
			/*
			 * The SNOW-f8 CHA generates the sX and rX  context
			 * state variables, and overwrites the original  key in
			 * the key register even if using initfinal. This is
			 * an expected behavior and the resolution is to reload
			 * the KEY register
			 */

			/* Insert Cipher Key */
			KEY(KEY1, cipherdata->key_enc_flags,
			    PTR(cipherdata->key), cipherdata->keylen, WITH(0));

			PROTOCOL(OP_TYPE_ENCAP_PROTOCOL,
				 OP_PCLID_3G_RLC_PDU,
				 (uint16_t)cipherdata->algtype);

			break;

		case RLC_CIPHER_TYPE_NULL:
			insert_copy_frame_op(program,
					     cipherdata,
					     OP_TYPE_ENCAP_PROTOCOL);
			break;
		default:
			pr_debug("%s: Invalid encrypt algorithm selected: %d\n",
				 "cnstr_shdsc_rlc_encap",
				 cipherdata->algtype);
			return;
		}
		break;

	default:
		break;
	}

	PATCH_HDR(phdr, pdb_end);
	*bufsize = PROGRAM_FINALIZE();
}

/**
 * @details                  Function for creating a WCDMA RLC
 *                           decapsulation descriptor.
 * @ingroup                  sharedesc_group
 *
 * @param[in,out] descbuf    Pointer to buffer for descriptor construction.
 * @param[in,out] bufsize    Size of descriptor written. Once the function
 *                           returns, the value of this parameter can be used
 *                           for reclaiming the space that wasn't used for the
 *                           descriptor.
 * @param[in] ps             If 36/40bit addressing is desired, this parameter
 *                           must be non-zero.
 * @param[in] mode           Indicates if ACKed or non-ACKed mode is used
 * @param[in] hfn            Starting Hyper Frame Number to be used together
 *                           with the SN from the RLC frames.
 * @param[in] bearer         Radio bearer ID.
 * @param[in] direction      The direction of the RLC PDU (UL/DL).
 * @param[in] hfn_threshold  HFN value that once reached triggers a warning
 *                           from SEC that keys should be renegociated at the
 *                           earliest convenience.
 * @param[in] cipherdata     Pointer to block cipher transform definitions.
 *                           Valid algorithm values are those from
 *                           cipher_type_rlc enum.
 *
 * @note  @b descbuf must be large enough to contain a full 256 byte long
 *        descriptor; after the function returns, by subtracting the actual
 *        number of bytes used (using @b bufsize), the user can reuse the
 *        remaining buffer space for other purposes.
 *
 */
static inline void cnstr_shdsc_rlc_decap(uint32_t *descbuf,
		unsigned *bufsize,
		unsigned short ps,
		enum rlc_mode mode,
		uint32_t hfn,
		unsigned short bearer,
		unsigned short direction,
		uint32_t hfn_threshold,
		struct alginfo *cipherdata)
{
	struct program prg;
	struct program *program = &prg;
	struct rlc_pdb pdb;
	LABEL(pdb_end);
	LABEL(keyjmp);
	REFERENCE(phdr);
	REFERENCE(pkeyjmp);

	PROGRAM_CNTXT_INIT(descbuf, 0);

	if (ps)
		PROGRAM_SET_36BIT_ADDR();

	phdr = SHR_HDR(SHR_SERIAL, 0, WITH(0));

	memset(&pdb, 0, sizeof(struct rlc_pdb));

	/*
	 * Read options from user:  depending on sequence number length,
	 * the HFN and HFN threshold have different lengths.
	 */
	switch (mode) {
	case RLC_UNACKED_MODE:
		pdb.opt_res = RLC_PDB_OPT_SNS_UM;
		pdb.hfn_res = hfn << RLC_PDB_HFN_SHIFT_UM;
		pdb.hfn_thr_res = hfn_threshold << RLC_PDB_HFN_SHIFT_UM;
		break;

	case RLC_ACKED_MODE:
		pdb.opt_res = RLC_PDB_OPT_SNS_AM;
		pdb.hfn_res = hfn << RLC_PDB_HFN_SHIFT_AM;
		pdb.hfn_thr_res = hfn_threshold << RLC_PDB_HFN_SHIFT_AM;
		break;

	default:
		pr_debug("Invalid RLC mode setting in PDB\n");
		return;
	}

	pdb.bearer_dir_res = (uint32_t)
				((bearer << RLC_PDB_BEARER_SHIFT) |
				 (direction << RLC_PDB_DIR_SHIFT));

	/* copy PDB in descriptor*/
	COPY_DATA((uint8_t *)&pdb, sizeof(struct rlc_pdb));

	SET_LABEL(pdb_end);

	if (insert_hfn_ov_op(program, mode, RLC_PDB_TYPE_FULL_PDB, 0))
		return;

	switch (mode) {
	case RLC_UNACKED_MODE:
	case RLC_ACKED_MODE:
		switch (cipherdata->algtype) {
		case RLC_CIPHER_TYPE_KASUMI:
			pkeyjmp = JUMP(IMM(keyjmp), LOCAL_JUMP, ALL_TRUE,
				       WITH(SHRD));
			/* Insert Cipher Key */
			KEY(KEY1, cipherdata->key_enc_flags,
			    PTR(cipherdata->key), cipherdata->keylen, WITH(0));
			SET_LABEL(keyjmp);

			PROTOCOL(OP_TYPE_DECAP_PROTOCOL,
				 OP_PCLID_3G_RLC_PDU,
				 (uint16_t)cipherdata->algtype);

			PATCH_JUMP(pkeyjmp, keyjmp);
			break;

		case RLC_CIPHER_TYPE_SNOW:
			/*
			 * The SNOW-f8 CHA generates the sX and rX  context
			 * state variables, and overwrites the original  key in
			 * the key register even if using initfinal. This is
			 * an expected behavior and the resolution is to reload
			 * the KEY register
			 */

			/* Insert Cipher Key */
			KEY(KEY1, cipherdata->key_enc_flags,
			    PTR(cipherdata->key), cipherdata->keylen, WITH(0));
			SET_LABEL(keyjmp);

			PROTOCOL(OP_TYPE_DECAP_PROTOCOL,
				 OP_PCLID_3G_RLC_PDU,
				 (uint16_t)cipherdata->algtype);

			break;

		case RLC_CIPHER_TYPE_NULL:
			insert_copy_frame_op(program,
					     cipherdata,
					     OP_TYPE_ENCAP_PROTOCOL);
			break;

		default:
			pr_debug("%s: Invalid encrypt algorithm selected: %d\n",
				 "cnstr_shdsc_rlc_decap",
				 cipherdata->algtype);
			return;
		}
		break;

	default:
		break;
	}

	PATCH_HDR(phdr, pdb_end);
	*bufsize = PROGRAM_FINALIZE();
}

#endif  /* __DESC_RLC_H__ */

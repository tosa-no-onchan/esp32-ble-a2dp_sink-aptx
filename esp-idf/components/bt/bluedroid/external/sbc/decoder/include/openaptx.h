/*
 * Open Source implementation of Audio Processing Technology codec (aptX)
 * Copyright (C) 2018  Pali Rohár <pali.rohar@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef OPENAPTX_H
#define OPENAPTX_H

#define OPENAPTX_MAJOR 0
#define OPENAPTX_MINOR 0
#define OPENAPTX_PATCH 0

struct aptx_context;

/*
 * Initialize context for aptX codec and reset it.
 * hd = 0 process aptX codec
 * hd = 1 process aptX HD codec
 */
struct aptx_context *aptx_init(int hd);

/*
 * Reset internal state, predictor and parity sync of aptX context.
 * It is needed when going to encode or decode a new stream.
 */
void aptx_reset(struct aptx_context *ctx);

/*
 * Free aptX context initialized by aptx_init().
 */
void aptx_finish(struct aptx_context *ctx);

/*
 * Encodes sequence of 4 raw 24bit signed stereo samples from input buffer with
 * size input_len to aptX audio samples into output buffer with size output_len.
 * Return value indicates processed length from input buffer and to written
 * pointer is stored length of encoded aptX audio samples in output buffer.
 * Therefore input buffer must contain sequence of the 24 bytes in format
 * LLLRRRLLLRRRLLLRRRLLLRRR (L-left, R-right) and output buffer would contain
 * encoded sequence of either four bytes (LLRR) of aptX or six bytes (LLLRRR)
 * of aptX HD. Due to aptX parity check it is suggested to provide multiple of
 * 8*4 raw input samples, therefore multiple of 8*24 bytes.
 */
size_t aptx_encode(struct aptx_context *ctx,
                   const unsigned char *input,
                   size_t input_len,
                   unsigned char *output,
                   size_t output_len,
                   size_t *written);

/*
 * Decodes aptX audio samples in input buffer with size input_len to sequence
 * of raw 24bit signed stereo samples into output buffer with size output_len.
 * Return value indicates processed length from input buffer and to written
 * pointer is stored length of decoded output samples in output buffer.
 * Input buffer must contain seqeunce of four bytes (LLRR) of aptX or six
 * bytes (LLLRRR) of aptX HD samples and output buffer would contain decoded
 * sequence of 24 bytes in format LLLRRRLLLRRRLLLRRRLLLRRR (L-left, R-right)
 * for one aptX sample. Due to aptX parity check it is suggested to provide
 * multiple of eight aptX samples, therefore multiple of 8*4 bytes for aptX
 * reps. 8*6 bytes for aptX HD.
 */
size_t aptx_decode(struct aptx_context *ctx,
                   const unsigned char *input,
                   size_t input_len,
                   unsigned char *output,
                   size_t output_len,
                   size_t *written);


size_t aptx_decode_16bit(struct aptx_context *ctx,
				const unsigned char *input,
				//unsigned char *input,
				size_t input_size,
				int16_t *output,
				size_t output_size,
				size_t *written);

int aptx_get_sample_size(struct aptx_context *ctx);

#endif

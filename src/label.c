/******************************************************************************
 *	Copyright (C) 2020	Alejandro Colomar Andrés		      *
 *	SPDX-License-Identifier:	GPL-2.0-only			      *
 ******************************************************************************/


/******************************************************************************
 ******* headers **************************************************************
 ******************************************************************************/
#include "label.h"

#include <stddef.h>

#define ALX_NO_PREFIX
#include <libalx/base/compiler/size.h>
#include <libalx/base/errno/perror.h>
#include <libalx/base/stdio/printf/sbprintf.h>
#include <libalx/base/stdio/seekc.h>
#include <libalx/base/stdlib/maximum.h>
#include <libalx/base/stdlib/minimum.h>
#include <libalx/base/stdlib/strto/strtoi_s.h>
#include <libalx/extra/cv/alx/gray.h>
#include <libalx/extra/cv/alx/median.h>
#include <libalx/extra/cv/core/array/bitwise.h>
#include <libalx/extra/cv/core/array/component.h>
#include <libalx/extra/cv/core/array/normalize.h>
#include <libalx/extra/cv/core/contours/contours.h>
#include <libalx/extra/cv/core/img/img.h>
#include <libalx/extra/cv/core/rect/rect.h>
#include <libalx/extra/cv/core/roi/roi.h>
#include <libalx/extra/cv/imgproc/features/edges.h>
#include <libalx/extra/cv/imgproc/filter/dilate_erode.h>
#include <libalx/extra/cv/imgproc/filter/smooth.h>
#include <libalx/extra/cv/imgproc/geometric/rotate.h>
#include <libalx/extra/cv/imgproc/miscellaneous/fill.h>
#include <libalx/extra/cv/imgproc/miscellaneous/threshold.h>
#include <libalx/extra/cv/imgproc/shape/contours.h>
#include <libalx/extra/cv/imgproc/shape/rect.h>
#include <libalx/extra/cv/types.h>

#include "dbg.h"


/******************************************************************************
 ******* macros ***************************************************************
 ******************************************************************************/


/******************************************************************************
 ******* enum / struct / union ************************************************
 ******************************************************************************/


/******************************************************************************
 ******* static prototypes ****************************************************
 ******************************************************************************/


/******************************************************************************
 ******* global functions *****************************************************
 ******************************************************************************/
int	find_label			(img_s *img)
{
	img_s		*tmp;
	conts_s		*conts;
	const cont_s	*lbl;
	rect_rot_s	*rect_rot;
	rect_s		*rect;
	int		status;

	/* init */
	status	= -1;
	if (alx_cv_init_img(&tmp))
		return	status;
	if (alx_cv_init_conts(&conts))
		goto err0;
	if (alx_cv_init_rect_rot(&rect_rot))
		goto err1;
	if (alx_cv_init_rect(&rect))
		goto err2;

	/* Find label */
	status--;
	alx_cv_clone(tmp, img);					dbg_show(2, tmp);
	alx_cv_white_mask(tmp, 50, 60, 40);			dbg_show(3, tmp);
	alx_cv_dilate_erode(tmp, 10);				dbg_show(3, tmp);
	alx_cv_erode_dilate(tmp, 30);				dbg_show(3, tmp);
	alx_cv_contours(tmp, conts);				dbg_show(2, tmp);
	if (alx_cv_conts_largest(&lbl, NULL, conts))
		goto err;
	alx_cv_min_area_rect(rect_rot, lbl);

	/* Align & crop to label */
	status--;
	alx_cv_rotate_2rect(img, rect_rot, rect);		dbg_show(3, img);
	alx_cv_roi_set(img, rect);		dbg_update_win(); dbg_show(1, img);

	/* deinit */
	status	= 0;
err:	alx_cv_deinit_rect(rect);
err2:	alx_cv_deinit_rect_rot(rect_rot);
err1:	alx_cv_deinit_conts(conts);
err0:	alx_cv_deinit_img(tmp);
	return	status;
}

int	find_symbols_vertically		(img_s *img)
{
	img_s		*clean, *tmp, *bkgd;
	conts_s		*conts;
	const cont_s	*syms;
	rect_s		*rect;
	ptrdiff_t	x, y, w, h;
	int		status;

	/* init */
	status	= -1;
	if (alx_cv_init_img(&clean))
		return	status;
	if (alx_cv_init_img(&bkgd))
		goto err0;
	if (alx_cv_init_img(&tmp))
		goto err1;
	if (alx_cv_init_conts(&conts))
		goto err2;
	if (alx_cv_init_rect(&rect))
		goto err3;

	/* Clean BKGD */
	status--;
	alx_cv_clone(tmp, img);					dbg_show(2, tmp);
	alx_cv_component(img, ALX_CV_CMP_BGR_R);		dbg_show(3, img);
	alx_cv_smooth(img, ALX_CV_SMOOTH_MEDIAN, 3);		dbg_show(3, img);
	alx_cv_clone(bkgd, img);				dbg_show(2, bkgd);
	alx_cv_clone(clean, img);				dbg_show(3, clean);
	alx_cv_white_mask(tmp, -1, 32, 64);			dbg_show(3, tmp);
	alx_cv_dilate_erode(tmp, 5);				dbg_show(3, tmp);
	alx_cv_bkgd_mask(tmp);					dbg_show(3, tmp);
	alx_cv_dilate(tmp, 10);					dbg_show(3, tmp);
	alx_cv_median(bkgd);					dbg_show(3, bkgd);
	alx_cv_and_2ref(bkgd, tmp);				dbg_show(3, bkgd);
	alx_cv_invert(tmp);					dbg_show(3, tmp);
	alx_cv_and_2ref(clean, tmp);				dbg_show(3, clean);
	alx_cv_or_2ref(clean, bkgd);				dbg_show(2, clean);

	/* Find syms */
	status--;
	alx_cv_clone(tmp, clean);				dbg_show(3, tmp);
	alx_cv_normalize(tmp);					dbg_show(3, tmp);
	alx_cv_smooth(tmp, ALX_CV_SMOOTH_MEDIAN, 5);		dbg_show(3, tmp);
	alx_cv_canny(tmp, 127, 200, 3, true);			dbg_show(3, tmp);
	alx_cv_dilate_h(tmp, 1);				dbg_show(3, tmp);
	alx_cv_dilate(tmp, 1);					dbg_show(3, tmp);
	alx_cv_holes_fill(tmp);					dbg_show(3, tmp);
	alx_cv_erode_dilate(tmp, 15);				dbg_show(3, tmp);
	alx_cv_extract_imgdata(tmp, NULL, &w, NULL, NULL, NULL, NULL);
	alx_cv_dilate_h(tmp, w / 6);				dbg_show(3, tmp);
	alx_cv_contours(tmp, conts);				dbg_show(2, tmp);
	if (alx_cv_conts_largest(&syms, NULL, conts))
		goto err; 
	alx_cv_bounding_rect(rect, syms);
	alx_cv_extract_rect(rect, NULL, &y, NULL, &h);

	/* Crop to symbols */
	status--;
	y	+= h / 2;
	h	*= 2;
	y	-= h / 2;
	x	= 0;
	if (alx_cv_set_rect(rect, x, y, w, h))
		goto err;
	alx_cv_roi_set(img, rect);		dbg_update_win(); dbg_show(1, img);

	/* deinit */
	status	= 0;
err:	alx_cv_deinit_rect(rect);
err3:	alx_cv_deinit_conts(conts);
err2:	alx_cv_deinit_img(tmp);
err1:	alx_cv_deinit_img(bkgd);
err0:	alx_cv_deinit_img(clean);
	return	status;
}

int	find_symbols_horizontally	(img_s *img)
{
	img_s		*tmp;
	conts_s		*conts;
	const cont_s	*syms;
	rect_s		*rect;
	ptrdiff_t	x, y, w, h;
	int		status;

	/* init */
	status	= -1;
	if (alx_cv_init_img(&tmp))
		return	status;
	if (alx_cv_init_conts(&conts))
		goto err0;
	if (alx_cv_init_rect(&rect))
		goto err1;

	/* Find symbols */
	status--;
	alx_cv_clone(tmp, img);					dbg_show(2, tmp);
	alx_cv_extract_imgdata(tmp, NULL, &w, &h, NULL, NULL, NULL);
	alx_cv_set_rect(rect, 20, 0, w - 40, h);
	alx_cv_roi_set(tmp, rect);				dbg_show(3, tmp);
	alx_cv_normalize(tmp);					dbg_show(3, tmp);
	alx_cv_threshold(tmp, ALX_CV_THRESH_BINARY_INV, ALX_CV_THR_OTSU);
								dbg_show(3, tmp);
	alx_cv_dilate_h(tmp, w / 40);				dbg_show(3, tmp);
	alx_cv_dilate_v(tmp, h / 40);				dbg_show(3, tmp);
	alx_cv_contours(tmp, conts);				dbg_show(2, tmp);
	if (alx_cv_conts_largest(&syms, NULL, conts))
		goto err;
	alx_cv_bounding_rect(rect, syms);
	alx_cv_extract_rect(rect, &x, NULL, &w, NULL);

	/* Crop to symbols */
	status--;
	y	= 0;
	w	+= 40;
	if (alx_cv_set_rect(rect, x, y, w, h))
		goto err;
	alx_cv_roi_set(img, rect);				dbg_show(1, img);

	/* deinit */
	status	= 0;
err:	alx_cv_deinit_rect(rect);
err1:	alx_cv_deinit_conts(conts);
err0:	alx_cv_deinit_img(tmp);
	return	status;
}

int	align_symbols			(img_s *img)
{
	img_s		*tmp;
	conts_s		*conts;
	const cont_s	*syms;
	rect_rot_s	*rect_rot;
	rect_s		*rect;
	ptrdiff_t	w, h;
	int		status;

	/* init */
	status	= -1;
	if (alx_cv_init_img(&tmp))
		return	status;
	if (alx_cv_init_conts(&conts))
		goto err0;
	if (alx_cv_init_rect_rot(&rect_rot))
		goto err1;
	if (alx_cv_init_rect(&rect))
		goto err2;

	/* Find symbols */
	status--;
	alx_cv_clone(tmp, img);					dbg_show(2, tmp);
	alx_cv_extract_imgdata(tmp, NULL, &w, &h, NULL, NULL, NULL);
	alx_cv_normalize(tmp);					dbg_show(3, tmp);
	alx_cv_threshold(tmp, ALX_CV_THRESH_BINARY_INV, ALX_CV_THR_OTSU);
								dbg_show(3, tmp);
	alx_cv_dilate_h(tmp, w / 40);				dbg_show(3, tmp);
	alx_cv_dilate_v(tmp, h / 40);				dbg_show(3, tmp);
	alx_cv_contours(tmp, conts);				dbg_show(2, tmp);
	if (alx_cv_conts_largest(&syms, NULL, conts))
		goto err;
	alx_cv_min_area_rect(rect_rot, syms);

	/* Aling & crop to symbols */
	alx_cv_rotate_2rect(img, rect_rot, rect);		dbg_show(3, img);
	alx_cv_roi_set(img, rect);				dbg_show(1, img);

	/* deinit */
	status	= 0;
err:	alx_cv_deinit_rect(rect);
err2:	alx_cv_deinit_rect_rot(rect_rot);
err1:	alx_cv_deinit_conts(conts);
err0:	alx_cv_deinit_img(tmp);
	return	status;
}


/******************************************************************************
 ******* static function definitions ******************************************
 ******************************************************************************/


/******************************************************************************
 ******* end of file **********************************************************
 ******************************************************************************/


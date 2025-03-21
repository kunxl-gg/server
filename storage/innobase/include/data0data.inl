/*****************************************************************************

Copyright (c) 1994, 2015, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2017, 2020, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/********************************************************************//**
@file include/data0data.ic
SQL data field and tuple

Created 5/30/1994 Heikki Tuuri
*************************************************************************/

#include "ut0rnd.h"

/*********************************************************************//**
Sets the type struct of SQL data field. */
UNIV_INLINE
void
dfield_set_type(
/*============*/
	dfield_t*	field,	/*!< in: SQL data field */
	const dtype_t*	type)	/*!< in: pointer to data type struct */
{
	ut_ad(field != NULL);
	ut_ad(type != NULL);

	field->type = *type;
}

/*********************************************************************//**
Sets length in a field. */
UNIV_INLINE
void
dfield_set_len(
/*===========*/
	dfield_t*	field,	/*!< in: field */
	ulint		len)	/*!< in: length or UNIV_SQL_NULL */
{
	ut_ad(len != UNIV_SQL_DEFAULT);
	field->ext = 0;
	field->len = static_cast<unsigned int>(len);
}

/** Gets spatial status for "external storage"
@param[in,out]	field		field */
UNIV_INLINE
spatial_status_t
dfield_get_spatial_status(
	const dfield_t*	field)
{
	ut_ad(dfield_is_ext(field));

	return(static_cast<spatial_status_t>(field->spatial_status));
}

/** Sets spatial status for "external storage"
@param[in,out]	field		field
@param[in]	spatial_status	spatial status */
UNIV_INLINE
void
dfield_set_spatial_status(
	dfield_t*		field,
	spatial_status_t	spatial_status)
{
	field->spatial_status = spatial_status & 3;
	ut_ad(dfield_get_spatial_status(field) == spatial_status);
}

/*********************************************************************//**
Sets pointer to the data and length in a field. */
UNIV_INLINE
void
dfield_set_data(
/*============*/
	dfield_t*	field,	/*!< in: field */
	const void*	data,	/*!< in: data */
	ulint		len)	/*!< in: length or UNIV_SQL_NULL */
{
	field->data = (void*) data;
	field->ext = 0;
	field->len = static_cast<unsigned int>(len);
}

/*********************************************************************//**
Sets pointer to the data and length in a field. */
UNIV_INLINE
void
dfield_write_mbr(
/*=============*/
	dfield_t*	field,	/*!< in: field */
	const double*	mbr)	/*!< in: data */
{
	MEM_CHECK_DEFINED(mbr, sizeof *mbr);
	field->ext = 0;

	for (unsigned i = 0; i < SPDIMS * 2; i++) {
		mach_double_write(static_cast<byte*>(field->data)
				  + i * sizeof(double), mbr[i]);
	}

	field->len = DATA_MBR_LEN;
}

/*********************************************************************//**
Sets a data field to SQL NULL. */
UNIV_INLINE
void
dfield_set_null(
/*============*/
	dfield_t*	field)	/*!< in/out: field */
{
	dfield_set_data(field, NULL, UNIV_SQL_NULL);
}

/*********************************************************************//**
Copies the data and len fields. */
UNIV_INLINE
void
dfield_copy_data(
/*=============*/
	dfield_t*	field1,	/*!< out: field to copy to */
	const dfield_t*	field2)	/*!< in: field to copy from */
{
	ut_ad(field1 != NULL);
	ut_ad(field2 != NULL);

	field1->data = field2->data;
	field1->len = field2->len;
	field1->ext = field2->ext;
	field1->spatial_status = field2->spatial_status;
}

/*********************************************************************//**
Copies a data field to another. */
UNIV_INLINE
void
dfield_copy(
/*========*/
	dfield_t*	field1,	/*!< out: field to copy to */
	const dfield_t*	field2)	/*!< in: field to copy from */
{
	*field1 = *field2;
}

/*********************************************************************//**
Copies the data pointed to by a data field. */
UNIV_INLINE
void
dfield_dup(
/*=======*/
	dfield_t*	field,	/*!< in/out: data field */
	mem_heap_t*	heap)	/*!< in: memory heap where allocated */
{
	if (!dfield_is_null(field)) {
		MEM_CHECK_DEFINED(field->data, field->len);
		field->data = mem_heap_dup(heap, field->data, field->len);
	}
}

/*********************************************************************//**
Tests if two data fields are equal.
If len==0, tests the data length and content for equality.
If len>0, tests the first len bytes of the content for equality.
@return TRUE if both fields are NULL or if they are equal */
UNIV_INLINE
ibool
dfield_datas_are_binary_equal(
/*==========================*/
	const dfield_t*	field1,	/*!< in: field */
	const dfield_t*	field2,	/*!< in: field */
	ulint		len)	/*!< in: maximum prefix to compare,
				or 0 to compare the whole field length */
{
	ulint	len2 = len;

	if (field1->len == UNIV_SQL_NULL || len == 0 || field1->len < len) {
		len = field1->len;
	}

	if (field2->len == UNIV_SQL_NULL || len2 == 0 || field2->len < len2) {
		len2 = field2->len;
	}

	return(len == len2
	       && (len == UNIV_SQL_NULL
		   || !memcmp(field1->data, field2->data, len)));
}

/*********************************************************************//**
Tests if dfield data length and content is equal to the given.
@return TRUE if equal */
UNIV_INLINE
ibool
dfield_data_is_binary_equal(
/*========================*/
	const dfield_t*	field,	/*!< in: field */
	ulint		len,	/*!< in: data length or UNIV_SQL_NULL */
	const byte*	data)	/*!< in: data */
{
	ut_ad(len != UNIV_SQL_DEFAULT);
	return(len == dfield_get_len(field)
	       && (!len || len == UNIV_SQL_NULL
		   || !memcmp(dfield_get_data(field), data, len)));
}

/*********************************************************************//**
Gets info bits in a data tuple.
@return info bits */
UNIV_INLINE
ulint
dtuple_get_info_bits(
/*=================*/
	const dtuple_t*	tuple)	/*!< in: tuple */
{
	return(tuple->info_bits);
}

/*********************************************************************//**
Sets info bits in a data tuple. */
UNIV_INLINE
void
dtuple_set_info_bits(
/*=================*/
	dtuple_t*	tuple,		/*!< in: tuple */
	ulint		info_bits)	/*!< in: info bits */
{
	tuple->info_bits = byte(info_bits);
}

/*********************************************************************//**
Gets number of fields used in record comparisons.
@return number of fields used in comparisons in rem0cmp.* */
UNIV_INLINE
uint16_t
dtuple_get_n_fields_cmp(
/*====================*/
	const dtuple_t*	tuple)	/*!< in: tuple */
{
	return(tuple->n_fields_cmp);
}

/*********************************************************************//**
Sets number of fields used in record comparisons. */
UNIV_INLINE
void
dtuple_set_n_fields_cmp(
/*====================*/
	dtuple_t*	tuple,		/*!< in: tuple */
	ulint		n_fields_cmp)	/*!< in: number of fields used in
					comparisons in rem0cmp.* */
{
	ut_ad(n_fields_cmp <= tuple->n_fields);
	tuple->n_fields_cmp = uint16_t(n_fields_cmp);
}

/** Creates a data tuple from an already allocated chunk of memory.
The size of the chunk must be at least DTUPLE_EST_ALLOC(n_fields).
The default value for number of fields used in record comparisons
for this tuple is n_fields.
@param[in,out]	buf		buffer to use
@param[in]	buf_size	buffer size
@param[in]	n_fields	number of field
@param[in]	n_v_fields	number of fields on virtual columns
@return created tuple (inside buf) */
UNIV_INLINE
dtuple_t*
dtuple_create_from_mem(
	void*	buf,
	ulint	buf_size,
	ulint	n_fields,
	ulint	n_v_fields)
{
	dtuple_t*	tuple;
	ulint		n_t_fields = n_fields + n_v_fields;

	ut_a(buf_size >= DTUPLE_EST_ALLOC(n_t_fields));

	tuple = (dtuple_t*) buf;
	tuple->info_bits = 0;
	tuple->n_fields = uint16_t(n_fields);
	tuple->n_v_fields = uint16_t(n_v_fields);
	tuple->n_fields_cmp = uint16_t(n_fields);
	tuple->fields = (dfield_t*) &tuple[1];
	if (n_v_fields > 0) {
		tuple->v_fields = &tuple->fields[n_fields];
	} else {
		tuple->v_fields = NULL;
	}

#ifdef UNIV_DEBUG
	tuple->magic_n = DATA_TUPLE_MAGIC_N;

	{	/* In the debug version, initialize fields to an error value */
		ulint	i;

		for (i = 0; i < n_t_fields; i++) {
			dfield_t*       field;

			if (i >= n_fields) {
				field = dtuple_get_nth_v_field(
					tuple, i - n_fields);
			} else {
				field = dtuple_get_nth_field(tuple, i);
			}

			dfield_set_len(field, UNIV_SQL_NULL);
			field->data = &data_error;
			dfield_get_type(field)->mtype = DATA_ERROR;
			dfield_get_type(field)->prtype = DATA_ERROR;
		}
	}
#endif
	MEM_CHECK_ADDRESSABLE(tuple->fields, n_t_fields
			      * sizeof *tuple->fields);
	MEM_UNDEFINED(tuple->fields, n_t_fields * sizeof *tuple->fields);
	return(tuple);
}

/** Duplicate the virtual field data in a dtuple_t
@param[in,out]		vrow	dtuple contains the virtual fields
@param[in,out]		heap	heap memory to use */
UNIV_INLINE
void
dtuple_dup_v_fld(dtuple_t* vrow, mem_heap_t* heap)
{
	for (ulint i = 0; i < vrow->n_v_fields; i++) {
		dfield_t*       dfield = dtuple_get_nth_v_field(vrow, i);
		dfield_dup(dfield, heap);
	}
}

/** Initialize the virtual field data in a dtuple_t
@param[in,out]		vrow	dtuple contains the virtual fields */
UNIV_INLINE
void
dtuple_init_v_fld(dtuple_t* vrow)
{
	for (ulint i = 0; i < vrow->n_v_fields; i++) {
		dfield_t*       dfield = dtuple_get_nth_v_field(vrow, i);
		dfield_get_type(dfield)->mtype = DATA_MISSING;
		dfield_set_len(dfield, UNIV_SQL_NULL);
	}
}

/**********************************************************//**
Creates a data tuple to a memory heap. The default value for number
of fields used in record comparisons for this tuple is n_fields.
@return own: created tuple */
UNIV_INLINE
dtuple_t*
dtuple_create(
/*==========*/
	mem_heap_t*	heap,	/*!< in: memory heap where the tuple
				is created, DTUPLE_EST_ALLOC(n_fields)
				bytes will be allocated from this heap */
	ulint		n_fields) /*!< in: number of fields */
{
	return(dtuple_create_with_vcol(heap, n_fields, 0));
}

/** Creates a data tuple with virtual columns to a memory heap.
@param[in]	heap		memory heap where the tuple is created
@param[in]	n_fields	number of fields
@param[in]	n_v_fields	number of fields on virtual col
@return own: created tuple */
UNIV_INLINE
dtuple_t*
dtuple_create_with_vcol(
	mem_heap_t*	heap,
	ulint		n_fields,
	ulint		n_v_fields)
{
	void*		buf;
	ulint		buf_size;
	dtuple_t*	tuple;

	ut_ad(heap);

	buf_size = DTUPLE_EST_ALLOC(n_fields + n_v_fields);
	buf = mem_heap_alloc(heap, buf_size);

	tuple = dtuple_create_from_mem(buf, buf_size, n_fields, n_v_fields);

	return(tuple);
}

inline void dtuple_set_n_fields(dtuple_t *tuple, ulint n_fields)
{
  tuple->n_fields= uint16_t(n_fields);
  tuple->n_fields_cmp= uint16_t(n_fields);
}

/** Copies a data tuple's virtual fields to another. This is a shallow copy;
@param[in,out]	d_tuple		destination tuple
@param[in]	s_tuple		source tuple */
UNIV_INLINE
void
dtuple_copy_v_fields(
	dtuple_t*	d_tuple,
	const dtuple_t*	s_tuple)
{

	ulint		n_v_fields	= dtuple_get_n_v_fields(d_tuple);
	ut_ad(n_v_fields == dtuple_get_n_v_fields(s_tuple));

	for (ulint i = 0; i < n_v_fields; i++) {
		dfield_copy(dtuple_get_nth_v_field(d_tuple, i),
			    dtuple_get_nth_v_field(s_tuple, i));
	}
}

/*********************************************************************//**
Copies a data tuple to another.  This is a shallow copy; if a deep copy
is desired, dfield_dup() will have to be invoked on each field.
@return own: copy of tuple */
UNIV_INLINE
dtuple_t*
dtuple_copy(
/*========*/
	const dtuple_t*	tuple,	/*!< in: tuple to copy from */
	mem_heap_t*	heap)	/*!< in: memory heap
				where the tuple is created */
{
	ulint		n_fields	= dtuple_get_n_fields(tuple);
	ulint		n_v_fields	= dtuple_get_n_v_fields(tuple);
	dtuple_t*	new_tuple	= dtuple_create_with_vcol(
		heap, tuple->n_fields, tuple->n_v_fields);
	ulint		i;

	for (i = 0; i < n_fields; i++) {
		dfield_copy(dtuple_get_nth_field(new_tuple, i),
			    dtuple_get_nth_field(tuple, i));
	}

	for (i = 0; i < n_v_fields; i++) {
		dfield_copy(dtuple_get_nth_v_field(new_tuple, i),
			    dtuple_get_nth_v_field(tuple, i));
	}

	return(new_tuple);
}

/**********************************************************//**
The following function returns the sum of data lengths of a tuple. The space
occupied by the field structs or the tuple struct is not counted. Neither
is possible space in externally stored parts of the field.
@return sum of data lengths */
UNIV_INLINE
ulint
dtuple_get_data_size(
/*=================*/
	const dtuple_t*	tuple,	/*!< in: typed data tuple */
	ulint		comp)	/*!< in: nonzero=ROW_FORMAT=COMPACT  */
{
	const dfield_t*	field;
	ulint		n_fields;
	ulint		len;
	ulint		i;
	ulint		sum	= 0;

	ut_ad(dtuple_check_typed(tuple));
	ut_ad(tuple->magic_n == DATA_TUPLE_MAGIC_N);

	n_fields = tuple->n_fields;

	for (i = 0; i < n_fields; i++) {
		field = dtuple_get_nth_field(tuple,  i);
		len = dfield_get_len(field);

		if (len == UNIV_SQL_NULL) {
			len = dtype_get_sql_null_size(dfield_get_type(field),
						      comp);
		}

		sum += len;
	}

	return(sum);
}

/*********************************************************************//**
Computes the number of externally stored fields in a data tuple.
@return number of externally stored fields */
UNIV_INLINE
ulint
dtuple_get_n_ext(
/*=============*/
	const dtuple_t*	tuple)	/*!< in: tuple */
{
	ulint	n_ext		= 0;
	ulint	n_fields	= tuple->n_fields;
	ulint	i;

	ut_ad(dtuple_check_typed(tuple));
	ut_ad(tuple->magic_n == DATA_TUPLE_MAGIC_N);

	for (i = 0; i < n_fields; i++) {
		n_ext += dtuple_get_nth_field(tuple, i)->ext;
	}

	return(n_ext);
}

/*******************************************************************//**
Sets types of fields binary in a tuple. */
UNIV_INLINE
void
dtuple_set_types_binary(
/*====================*/
	dtuple_t*	tuple,	/*!< in: data tuple */
	ulint		n)	/*!< in: number of fields to set */
{
	dtype_t*	dfield_type;
	ulint		i;

	for (i = 0; i < n; i++) {
		dfield_type = dfield_get_type(dtuple_get_nth_field(tuple, i));
		dtype_set(dfield_type, DATA_BINARY, 0, 0);
	}
}

/**********************************************************************//**
Writes an SQL null field full of zeros. */
UNIV_INLINE
void
data_write_sql_null(
/*================*/
	byte*	data,	/*!< in: pointer to a buffer of size len */
	ulint	len)	/*!< in: SQL null size in bytes */
{
	memset(data, 0, len);
}

/** Checks if a dtuple contains an SQL null value.
@param tuple tuple
@param fields_number number of fields in the tuple to check
@return true if some field is SQL null */
UNIV_INLINE
bool dtuple_contains_null(const dtuple_t *tuple, ulint fields_number)
{
  ulint n= fields_number ? fields_number : dtuple_get_n_fields(tuple);
  for (ulint i= 0; i < n; i++)
    if (dfield_is_null(dtuple_get_nth_field(tuple, i)))
      return true;
  return false;
}

/**************************************************************//**
Frees the memory in a big rec vector. */
UNIV_INLINE
void
dtuple_big_rec_free(
/*================*/
	big_rec_t*	vector)	/*!< in, own: big rec vector; it is
				freed in this function */
{
	mem_heap_free(vector->heap);
}

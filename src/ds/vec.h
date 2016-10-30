/*
 * Marcel the Shell -- a shell written in C
 * Copyright (C) 2016 Chad Sharp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef M_VEC_H
#define M_VEC_H

typedef void* vec;

vec vec_alloc(size_t size);
void vec_free(vec v);
size_t vec_capacity(vec v);
size_t vec_len(vec v);
void vec_setlen(size_t val, vec v);
int vec_append(void *elem, size_t elem_size, vec *v);
int vec_grow(vec *v);
#endif

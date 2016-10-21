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

vec valloc(size_t size);
void vfree(vec v);
size_t vcapacity(vec v);
size_t vlen(vec v);
void vsetlen(size_t val, vec v);
int vappend(void *elem, size_t elem_size, vec *v);
int vgrow(vec *v);
#endif

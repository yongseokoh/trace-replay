/****************************************************************************
 * Block I/O Trace Replayer 
 * Yongseok Oh (ysoh@uos.ac.kr) 2013 - 2014

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#ifndef _SGIO_H
#define _SGIO_H
int do_drive_cmd (int fd, unsigned char *args);

#endif

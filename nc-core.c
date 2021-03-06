/*
 *	ncurses additional tools
 * 
 *	Copyright (C) 2017-2021 Nicholas Christopoulos.
 *
 *	This is free software: you can redistribute it and/or modify it under
 *	the terms of the GNU General Public License as published by the
 *	Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	It is distributed in the hope that it will be useful, but WITHOUT ANY
 *	WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *	for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with it. If not, see <http://www.gnu.org/licenses/>.
 *
 * 	Written by Nicholas Christopoulos <nereus@freemail.gr>
 */

#include "nc-lib.h"

#define is_bold(c) ((1 << 3) & (c))

// display a title on window 'win'
// align can be 0: left, 1: right, 2: center
void nc_wtitle(WINDOW *win, const char * title, int align) {
	wattron(win, A_BOLD);
	switch ( align ) {
	case 0:	// left
		mvwprintw(win, 0, 2, " %s ", title);
		break;
	case 1: // right
		mvwprintw(win, 0, getmaxx(win) - (strlen(title)+2) - 2, " %s ", title);
		break;
	case 2: // center
		mvwprintw(win, 0, getmaxx(win) / 2 - (strlen(title) + 2) / 2, " %s ", title);
		break;
		}
	wattroff(win, A_BOLD);
	}

// hexadecimal character to integer
static int c2dec(char c) {
	c = toupper(c);
	if ( c >= 'A' && c <= 'F' )	return 10 + (c - 'A');
	return (c - '0');
	}

static int nc_colornum(int fg, int bg) {
	int B, bbb, ffff;

	B = 1 << 7;
	bbb = (7 & bg) << 4;
	ffff = 7 & fg;
	return (B | bbb | ffff);
	}

static short nc_ccolor(int fg) {
	switch (7 & fg)	{           /* RGB */
	case 0: return (COLOR_BLACK);
	case 1: return (COLOR_BLUE);
	case 2: return (COLOR_GREEN);
	case 3: return (COLOR_CYAN);
	case 4: return (COLOR_RED);
	case 5: return (COLOR_MAGENTA);
	case 6: return (COLOR_YELLOW);
	case 7: return (COLOR_WHITE);
		}
	return 0;
	}

static void nc_clrpairs() {
	int fg, bg;
	int colorpair;

	for ( bg = 0; bg <= 7; bg ++ ) {
		for ( fg = 0; fg <= 7; fg ++ ) {
			colorpair = nc_colornum(fg, bg);
			init_pair(colorpair, nc_ccolor(fg), nc_ccolor(bg));
			}
		}
	}

// initialize ncurses
void nc_init() {
	initscr();
	noecho(); nonl(); cbreak();
	keypad(stdscr, TRUE);
	keypad(curscr, TRUE);
	curs_set(0);

    if ( has_colors() ) {
		start_color();
		nc_clrpairs();
		}
	}

// close ncurses
void nc_close() {
	endwin(); 
	}

void nc_color_on(WINDOW *win, int fg, int bg) {	/* set the color pair (colornum) and bold/bright (A_BOLD) */
	wattron(win, COLOR_PAIR(nc_colornum(fg, bg)));
	if ( is_bold(fg) ) wattron(win, A_BOLD);
	}

void nc_color_off(WINDOW *win, int fg, int bg) {	/* unset the color pair (colornum) and bold/bright (A_BOLD) */
	wattroff(win, COLOR_PAIR(nc_colornum(fg, bg)));
	if ( is_bold(fg) ) wattroff(win, A_BOLD);
	}

static void nc_hcon(WINDOW *win, char fg, char bg) { nc_color_on(win, c2dec(fg), c2dec(bg)); }
static void nc_hcoff(WINDOW *win, char fg, char bg) { nc_color_off(win, c2dec(fg), c2dec(bg)); }

// print with codes/colors
#define ccase(c,a) case (c): ( p[1] == '+' ) ? wattron(win, (a)) : wattroff(win, (a)); p ++; break;

void nc_mvwprintf(WINDOW *win, int y, int x, const char *fmt, ...) {
	char	msg[LINE_MAX], buf[LINE_MAX];
	char	*p = msg, *b = buf;

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, LINE_MAX, fmt, ap);
	va_end(ap);

	if ( y >= 0 && x >= 0 )
		wmove(win, y, x);
	else if ( y >= 0 )
		wmove(win, y, getcurx(win));
	else if ( x >= 0 )
		wmove(win, getcury(win), x);
		
	while ( *p ) {
		switch ( *p ) {
		case 27:
			p ++;
			if ( b > buf ) { *b = '\0'; waddstr(win, buf); b = buf; }
			switch ( *p ) {
			ccase('b', A_BOLD)
			ccase('r', A_REVERSE)
			ccase('d', A_DIM)
			ccase('u', A_UNDERLINE)
			case 'c':
				p ++;
				if ( isxdigit(*p) && isxdigit(p[1]) ) {
					if ( has_colors() ) {
						if ( p[2] == '+' )
							nc_hcon(win, p[0], p[1]);
						else
							nc_hcoff(win, p[0], p[1]);
						}
					p += 2;
					}
				break;
			default: // not recognized
				*b ++ = '\033';
				*b ++ = *p;
				}
			break;
		default:
			*b ++ = *p;
			}
		p ++;
		}
	if ( b > buf ) { *b = '\0'; waddstr(win, buf); b = buf; }
	}
#undef ccase

#define nc_wprintf(w,f,...)	nc_mvwprintf(w,-1,-1,f,__VA_ARGS__)
#define nc_printf(f,...)	nc_mvwprintf(stdscr,-1,-1,f,__VA_ARGS__)



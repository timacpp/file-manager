#pragma once

/** Displays errno and ends program */
extern void syserr(const char* fmt, ...);

/** Displays text and ends program */
extern void fatal(const char* fmt, ...);

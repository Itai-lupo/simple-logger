/**
 * @file colors.h
 * @author itai lupo
 * @brief you can use this to add colors to your logs
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#define CEND "\33[0m"
#define CBOLD "\33[1m"
#define CITALIC "\33[3m"
#define CURL "\33[4m"
#define CBLINK "\33[5m"
#define CBLINK2 "\33[6m"
#define CSELECTED "\33[7m"
#define CBLACK "\33[30m"
#define CRED "\33[31m"
#define CGREEN "\33[32m"
#define CYELLOW "\33[33m"
#define CBLUE "\33[34m"
#define CVIOLET "\33[35m"
#define CBEIGE "\33[36m"
#define CWHITE "\33[37m"
#define CBLACKBG "\33[40m"
#define CREDBG "\33[41m"
#define CGREENBG "\33[42m"
#define CYELLOWBG "\33[43m"
#define CBLUEBG "\33[44m"
#define CVIOLETBG "\33[45m"
#define CBEIGEBG "\33[46m"
#define CWHITEBG "\33[47m"
#define CGREY "\33[90m"
#define CRED2 "\33[91m"
#define CGREEN2 "\33[92m"
#define CYELLOW2 "\33[93m"
#define CBLUE2 "\33[94m"
#define CVIOLET2 "\33[95m"
#define CBEIGE2 "\33[96m"
#define CWHITE2 "\33[97m"
#define CGREYBG "\33[100m"
#define CREDBG2 "\33[101m"
#define CGREENBG2 "\33[102m"
#define CYELLOWBG2 "\33[103m"
#define CBLUEBG2 "\33[104m"
#define CVIOLETBG2 "\33[105m"
#define CBEIGEBG2 "\33[106m"
#define CWHITEBG2 "\33[107m"

/**
 * @brief this is the color scheme that will be used for the different log levels.
 */
static char logLevelColors[6][18] = {
	CGREY, CBLUE, CGREEN CBOLD, CYELLOW CBOLD, CRED CBOLD, CWHITE2 CBOLD CREDBG,
};

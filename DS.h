/* DS.h */
// Contains relavant Data structure definitions and utility functions for the program

#pragma once

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>

using namespace std;

/* --------- Data Structure definitions  --------- */
struct countryData
{
	int numCases = 0;
	int numDeaths = 0;
	int numRecoveries = 0;
};

typedef map<string, countryData> innerMap;

// *Value* for map
struct myStruct
{
	innerMap m;
	string firstConfirmed = "";
	string firstDeath = "";
	string firstRecovery = "";
	double lifeExpectancy = 0.0;
	long population = 0;
};

// Data Structure to hold country data for the program
typedef map<string, myStruct> myMap; 

typedef pair<string, int> myPair;  // Defines (Country, Confirmed) pair for top10
typedef pair<int, long> modelPair; // Defines (# of day, Confirmed) pair for model 

// comparator for partial_sort
// compares the number of confirmed cases for two (country, confirmed) pairs
struct sortPairSecond
{
	bool operator() (const myPair &lhs, const myPair &rhs)
	{
		return lhs.second > rhs.second;
	}
};
/* --------------------------------------------- */

/* ---------    Utility Functions     --------- */

//
// getCurrentDay
//
// Returns the day number
//
int getDay(string date)
{
	int dayNum = 0;
	string month, day;
	size_t pos = date.find("-");
	month = date.substr(0, pos);
	date.erase(0, pos + 1);

	pos = date.find("-");
	day = date.substr(0, pos);

	if (month == "01")
		dayNum = stoi(day) - 21;
	else if (month == "02")
		dayNum = 10 + stoi(day);
	else if (month == "03")
		dayNum = 39 + stoi(day);

	return dayNum;
}

//
// getDateDifference
//
// Returns the difference (number of days in between) of 2 dates
//
int getDateDifference(string currentDate, string otherDate)
{
	int day1 = getDay(currentDate);
	int day2 = getDay(otherDate);

	return (day1 - day2);
}


//
// parseDate: 
//
// Extracts the date from the file name
//
string parseDate(string date)
{
	size_t pos = date.find("0");
	date.erase(0, pos);

	pos = date.find(".");
	date.erase(pos);

	return date;
}

//
// displayCorrectNumber:
//
// displays the # of confirmed, cases, deaths or recovered based upon the option
//
int displayCorrectNumber(innerMap::iterator itr, string option)
{
	if (option == "c")
		return itr->second.numCases;

	else if (option == "d")
		return itr->second.numDeaths;

	else if (option == "r")
		return itr->second.numRecoveries;
}

/* ------------------------------------------------ */
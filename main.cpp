/*main.cpp*/

//
// COVID-19 Data Analysis Program
// Author: Nishant Chudasama
// 

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <filesystem>
#include <locale>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <set>
#include <cmath>
#include "DS.h"

using namespace std;
namespace fs = std::filesystem;

int string2int(string convert) {
	try {
		return stoi(convert);
	}
	catch (exception& e) {
		return 0;
	}
}


//
// getFilesWithinFolder:
//
// Given the path to a folder, e.g. "./daily_reports/", returns 
// a vector containing the full pathnames of all regular files
// in this folder.  If the folder is empty, the vector is empty.
//
vector<string> getFilesWithinFolder(string folderPath, string &currentDate)
{
	vector<string> files;

	for (const auto& entry : fs::directory_iterator(folderPath))
	{
		if (entry.is_regular_file())
		{
			// this gives you just the filename:
			// files.push_back(entry.path().filename().string());

			// this gives you the full path + filename:
			files.push_back(entry.path().string());
		}
	}

	// let's make sure the files are in alphabetical order, so we 
	// process them in the correct order by date:
	std::sort(files.begin(), files.end());

	currentDate = parseDate(files.back());
	
	return files;
}

//
// parseAndStoreDate:
//
// Function to parse data in .csv file and store data into map
//
void parseAndStoreDate(string& line, myMap &M, string date, long &dailyConfirmed)	
{
	// Convert from "City, State" to City State
	if (line[0] == '"') {
		line.erase(0, 1);             // erase leading "
		size_t pos = line.find(',');  // find embedded ','
		line.erase(pos, 1);           // delete ','
		pos = line.find('"');         // find closing "
		line.erase(pos, 1);           // delete closing "
	}

	stringstream s(line);
	string province, country, last_update;
	string confirmed, deaths, recovered;

	int numConfirmed, numDeaths, numRecovered;

	getline(s, province, ',');
	getline(s, country, ',');
	getline(s, last_update, ',');
	getline(s, confirmed, ',');
	getline(s, deaths, ',');
	getline(s, recovered, ',');

	if (confirmed == "")
		confirmed = "0";

	if (deaths == "")
		deaths = "0";

	if (recovered == "")
		recovered = "0";

	if (country == "Mainland China")
		country = "China";

	if (country == "Republic of Korea")
		country = "South Korea";

	numConfirmed = string2int(confirmed);
	numDeaths = string2int(deaths);
	numRecovered = string2int(recovered);

	dailyConfirmed += numConfirmed;


	// Store into map
	M[country].m[date].numCases += numConfirmed;
	M[country].m[date].numDeaths += numDeaths;
	M[country].m[date].numRecoveries += numRecovered;

	if (numConfirmed > 0 && M[country].firstConfirmed == "") {
		M[country].firstConfirmed = date;
	}

	if (numDeaths > 0 && M[country].firstDeath == "") {
		M[country].firstDeath = date;
	}

	if (numRecovered > 0 && M[country].firstRecovery == "") {
		M[country].firstRecovery = date;
	}
	
}

//
// getData:
//
// Function to open the .csv files and extract lines for storing into the map
// Also populates the vector for generating the model with (day,cases) pairs
//
void getData(vector<string> &files, myMap &M, int &numFiles, vector<modelPair>& modelVector)
{
	string line;
	string date;
	long dailyConfirmed;
	
	for (auto file : files)
	{	
		dailyConfirmed = 0;

		ifstream infile(file);

		if (!infile.good()) {
			cout << "Error opening " << file << endl;
			exit(-1);
		}

		numFiles++;

		getline(infile, line);    // Read and discard first line
		
		date = parseDate(file);
		
		while (getline(infile, line))
		{
			parseAndStoreDate(line, M, date, dailyConfirmed);
		}

		infile.close();
		modelVector.push_back(make_pair(getDay(date),dailyConfirmed));
		
	}
}

//
// getPopulations:
//
// Store populations for countries (164 countries in daily reports)
//
void getPopulations(myMap& M, int &numFactFiles)
{
	ifstream infile("./worldfacts/populations.csv");

	numFactFiles++;

	string line;
	getline(infile, line); // Read and discard first line
	int count = 0;

	string pos, country, population;

	while (getline(infile, line))
	{
		stringstream s(line);
		getline(s, pos, ',');
		getline(s, country, ',');
		getline(s, population);

		auto itr = M.find(country);
		if (itr != M.end()) {
			itr->second.population = long(string2int(population));
			count++;
		}
	}

	infile.close();
}

//
// getLifeExpectancies:
//
// Stores life expectancies for countries (159 in daily reports)
//
void getLifeExpectancies(myMap& M, int& numFactFiles)
{
	ifstream infile("./worldfacts/life_expectancies.csv");

	if (!infile.good()) {
		cout << "Could not open life_expectancies.csv";
		exit(-1);
	}

	numFactFiles++;

	string line;
	getline(infile, line);  // Read and discard first line
	int count = 0;

	while (getline(infile, line))
	{
		stringstream s(line);
		string pos, country, lifeExpectancy;

		getline(s, pos, ',');
		getline(s, country, ',');
		getline(s, lifeExpectancy);

		//cout << pos << " " << country << " " << population << endl;

		auto itr = M.find(country);
		if (itr != M.end()) {
			//cout << count << ". Adding life expectancy for " << country << endl;
			itr->second.lifeExpectancy = stod(lifeExpectancy);
			count++;
		}
	}

	infile.close();
}

void displayHelpMenu()
{
	cout << "Available commands:" << endl;
	cout << "<name>: enter a country name such as US or China" << endl;
	cout << "countries: list all countries and most recent report" << endl;
	cout << "top10: list of top 10 countries based on most recent # of confirmed cases" << endl;
	cout << "totals: world-wide totals of confirmed, deaths, recovered" << endl;
	cout << "model: generate exponential model for the number of confirmed cases worldwide" << endl;
	cout << endl;
}

void totals(string currentDate, myMap& M)			
{
	long confirmed = 0, deaths = 0, recovered = 0;
	double deathPercent, recoveredPercent;

	for (auto itr = M.begin(); itr != M.end(); ++itr) {
		confirmed += itr->second.m[currentDate].numCases;
		deaths += itr->second.m[currentDate].numDeaths;
		recovered += itr->second.m[currentDate].numRecoveries;
	}

	deathPercent = (deaths * 100.0) / confirmed;
	recoveredPercent = (recovered * 100.0) / confirmed;

	cout << "As of " << currentDate << ", the world-wide totals are:" << endl;
	cout << " confirmed: " << confirmed << endl;
	cout << " deaths: " << deaths << " (" << deathPercent << "%)" << endl;
	cout << " recovered: " << recovered << " (" << recoveredPercent << "%)" << endl;
	cout << endl;
}

//
// countries
//
// Display most recent confirmed, deaths and recovered for all countries in
// alphabetical order
//
void countries(myMap& M, string currentDate)
{
	for (auto itr = M.begin(); itr != M.end(); ++itr) {
		cout << itr->first << ": "; // Display country name

		cout << itr->second.m[currentDate].numCases << ", ";
		cout << itr->second.m[currentDate].numDeaths << ", ";
		cout << itr->second.m[currentDate].numRecoveries << endl;
	}

	cout << endl;
}

//
// top10
//
// Display the 10 countries with the greatest number of cases
//
void top10(myMap& M, string currentDate)
{
	vector<myPair> cases;  // Make vector of type myPair (pair<string, int>)

	// Insert (Country, Confirmed) pairs into vector
	for (auto itr = M.begin(); itr != M.end(); ++itr) {
		cases.push_back(make_pair(itr->first, itr->second.m[currentDate].numCases));
	}

	// Sort vector in descending order of *values*
	// sortPairSecond() defined in DS.h
	partial_sort(cases.begin(), cases.end(), cases.end(), sortPairSecond());

	// Display first 10 values in sorted vector
	int count = 1;
	for (auto itr = cases.begin(); itr < cases.begin() + 10; ++itr) {
		cout << count << ". " << itr->first << ": " << itr->second << endl;
		count++;
	}

	cout << endl;

}

//
// timeLine
//
// display timeline for confirmed, deaths or recovered cases for a given country
//
void timeLine(myMap& M, string country, string currentDate, string option)
{
	innerMap temp = M[country].m;
	string firstDate;
	string firstConfirmed = M[country].firstConfirmed;

	if (option == "c") {
		cout << "Confirmed:" << endl;
		firstDate = M[country].firstConfirmed;
	}
		
	else if (option == "d") {
		cout << "Deaths:" << endl;
		firstDate = M[country].firstDeath;
	}
		
	else if (option == "r") {
		cout << "Recovered:" << endl;
		firstDate = M[country].firstRecovery;
	}
	
	// There is no timeline
	if (firstDate == "")
		return;

	auto itr = temp.find(firstDate);
	int count = getDateDifference(firstDate, firstConfirmed) + 1;

	if (getDateDifference(currentDate, firstDate) > 14)
	{
		// First 7 countries
		for (int i = 0; i < 7; ++i)
		{
			cout << itr->first << " (day " << count << "): ";

			// Display # of cases, recovered or deaths depending on option
			cout << displayCorrectNumber(itr, option) << endl;

			++count;
			++itr;
		}

		while (count < (temp.size() - 6)) {
			++itr;
			++count;
		}

		cout << " ." << endl;
		cout << " ." << endl;
		cout << " ." << endl;

		// Last 7 countries:
		while (itr != temp.end()) {
			cout << itr->first << " (day " << count << "): ";

			// Display # of cases, recovered or deaths depending on option
			cout << displayCorrectNumber(itr, option) << endl;

			++itr;
			++count;
		}
	}

	else {
		while (itr != temp.end()) {
			cout << itr->first << " (day " << count << "): ";
			
			// Display # of cases, recovered or deaths depending on option
			cout << displayCorrectNumber(itr, option) << endl;

			++itr;
			++count;
		}
	}
}

//
// countryData
//
// Function to display statistics for a given country
//
void countryData(myMap& M, string country, string currentDate)
{
	string option;

	cout << "Population: " << M[country].population << endl;
	cout << "Life Expectancy: " << M[country].lifeExpectancy << " years" << endl;

	// Display data for the last entry in the inner map
	cout << "Latest Data: " << endl;
	auto itr = M[country].m.rbegin();
	cout << " confirmed: " << itr->second.numCases << endl;
	cout << " deaths: " << itr->second.numDeaths << endl;
	cout << " recovered: " << itr->second.numRecoveries << endl;
		
	cout << "First confirmed case: ";
	if (M[country].firstConfirmed != "")
		cout << M[country].firstConfirmed << endl;
	else
		cout << "none" << endl;

	cout << "First recorded death: ";
	if (M[country].firstDeath != "")
		cout << M[country].firstDeath << endl;
	else
		cout << "none" << endl;

	cout << "Do you want to see a timeline? Enter c/d/r/n> ";
	cin.sync();
	getline(cin, option);

	// Display timeline for confirmed/deaths/recovered cases
	if (option == "c" || option == "d" || option == "r")
		timeLine(M, country, currentDate, option);

	cout << endl;
}

//
// generateModel
// 
// Function to generate an exponential model for the number of cases worldwide
//
void generateModel(vector<modelPair> &modelVector, string currentDate)
{
	// Generate a simple exponential model for the number of cases
	// of the form: y = a*e^(bx)
	// or, ln y = ln a + bx, which is equivalent to Y = A + bX 

	// Using linear regression: 
	//  A = (Sx^2*Sy - Sx*Sxy) / (n*S(x^2) - (Sx)^2)
	//  b = (n*Sxy - Sx*Sy) / (n*S(x^2) - (Sx)^2) 
	//
	//  Where S represents the summation (Sigma)

	long long int sumx = 0, sumy = 0, sumxy = 0, sumx2 = 0, numDays;
	long double a, b, A;
	int x, y;
	int n = modelVector.size();

	for (auto& elem : modelVector)
	{
		x = elem.first;
		y = log(elem.second);

		sumx = sumx + x;
		sumy = sumy + y;
		sumxy = sumxy + (x * y);
		sumx2 = sumx2 + (x * x);

	}

	A = ((sumx2 * sumy - sumx * sumxy) * 1.0 / (n * sumx2 - sumx * sumx) * 1.0);
	b = ((n * sumxy - sumx * sumy) * 1.0 / (n * sumx2 - sumx * sumx) * 1.0);
	a = exp(A);

	cout << "Data is modeled by: y = " << a << "e^" << b << "X" << endl << endl;

	// World Population as of 12:00 CDT 03/29/2020
	// Source: US and World Population Clock (US Census Bureau): https://www.census.gov/popclock/
	// World poulation is dynamic and might have changed since the time of writing this program
	long long int worldPopulation = 7639708031;

	numDays = (log(worldPopulation) - A) / b;
	numDays = numDays - getDay(currentDate);

	cout << "At current rate of infection (as of " << currentDate <<"):" << endl;
	cout << "Number of days required to infect the whole world: " << numDays << endl << endl;

	cout << "The above model is only a simple attempt of extrapolating data...." << endl;
	cout << "The spread of a real epidemic depends on a lot more factors "
		 << "and cannot be modeled by an exponential curve" << endl << endl;

	cout << "Follow social distancing measures and prevent the spread of the disease." << endl;
	cout << "Happy Quarantining!" << endl;

	cout << endl;
}

//
// main:
//
int main()
{
	myMap M;
	int numDailyReports = 0;
	int numFactFiles = 0;
	int numCountries = 0;
	int totalConfirmed = 0;
	int totalDeaths = 0;
	int totalRecovered = 0;
	string command;
	string currentDate;
	vector<modelPair> modelVector;

	cout << "** COVID-19 Data Analysis **" << endl;
	cout << endl;
	cout << "Based on data made available by John Hopkins University" << endl;
	cout << "https://github.com/CSSEGISandData/COVID-19" << endl;
	cout << endl;

	//
	// setup cout to use thousands separator, and 2 decimal places:
	//
	cout.imbue(std::locale(""));
	cout << std::fixed;
	cout << std::setprecision(2);

	//
	// get a vector of the daily reports, one filename for each:
	//
	vector<string> files = getFilesWithinFolder("./daily_reports/", currentDate);

	getData(files, M, numDailyReports, modelVector);
	getPopulations(M, numFactFiles);
	getLifeExpectancies(M, numFactFiles);

	cout << ">> Processed " << numDailyReports << " daily reports" << endl;
	cout << ">> Processed " << numFactFiles << " files of world facts" << endl;
	cout << ">> Current data on " << M.size() << " countries" << endl;
	cout << endl;

	cout << "Enter command (help for list, # to quit)> ";
	
	getline(cin, command);
	cin.sync();

	while (command != "#")
	{
		if (command == "help") {
			displayHelpMenu();
		}
		
		// Display total statistics for the world
		else if (command == "totals") {
			totals(currentDate, M);
		}

		// Display the confirmed, deaths and recoveries for all countries (alphabetical order)
		else if (command == "countries") {
			countries(M, currentDate);
		}

		// Command to display the 10 countries with the highest number of confirmed cases
		else if (command == "top10") {
			top10(M, currentDate);
		}

		// Command to display exponential model for the # of confirmed cases worldwide
		else if (command == "model") {
			generateModel(modelVector, currentDate);
		}

		// <name> command
		else if (M.find(command) != M.end()) {
			countryData(M, command, currentDate);
		}

		else {
			cout << "country or command not found..." << endl << endl;;
		}

		cout << "Enter command> ";
		cin.sync();
		getline(cin, command);
	}


	return 0;
}
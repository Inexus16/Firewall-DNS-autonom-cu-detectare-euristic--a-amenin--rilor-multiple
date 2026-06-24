#include "Generator.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <iostream> 
#include <fstream>
#include <set> 
#include <iterator> 

const char *tlds[10] = { "ru", "info", "biz", "click", "su", "work", "pl", "org", "pw", "xyz" };

int LockyDGA(char *domain, int pos, int seed, SYSTEMTIME systemTime)
{
	int v1;
	int v2;
	int v3;
	int v4;
	int v8;
	int v9;
	int v10;
	int v11;
	int v12;
	int v13;
	int v14;
	int v15;
	int v17;
	int v18;
	int v19;
	int v20;
	const char *v21;
	int v7;
	unsigned int v5;
	int v6;
	int var18;
	int var14;
	int var10;
	v1 = pos;
	v2 = seed;
	v3 = 0;
	v5 = systemTime.wDay >> 1;
	v4 = systemTime.wYear;
	v1 = _rotl(v1, 0x15);
	v6 = _rotl(v2, 0x11);
	var18 = v6 + v1;
	var14 = v5;
	var10 = 7;
	while (var10 > 0)
	{
		v7 = _rotr(0xB11924E1 * (v4 + v3 + 0x1BF5), 7);
		v8 = (v7 + 0x27100001) ^ v3;
		v9 = _rotr(0xB11924E1 * (v8 + v2), 7);
		v10 = (v9 + 0x27100001) ^ v8;
		v11 = _rotr(0xB11924E1 * (v5 + v10), 7);
		v12 = 0xD8EFFFFF - v11 + v10;
		v13 = _rotr(0xB11924E1 * (systemTime.wMonth + v12
			- 0x65CAD), 7); v14 = v12 + v13 + 0x27100001;
		v15 = _rotr(0xB11924E1 * (v14 + var18), 7);
		v3 = (v15 + 0x27100001) ^ v14;
		++v4;
		var10 = var10 - 1;
		v5 = var14;
	}
	var18 = v3 % 0xBu + 7;
	var10 = 0;
	if (var18 != 0)
	{
		do
		{
			v17 = _rotl(v3, var10);
			v18 = _rotr(0xB11924E1 * v17, 7);
			v3 = v18 + 0x27100001;
			domain[var10++] = v3 % 0x19u + 'a';
		} while (var10 < var18);
	}
	domain[var10++] = '.';
	v19 = _rotr(0xB11924E1 * v3, 7);
	v20 = 0;
	v21 = tlds[(v19 + 0x27100001) % (sizeof(tlds) /
		sizeof(tlds[0]))];
	do
	{
		if (!v21[v20])
		{
			break;
		}
		domain[var10++] = v21[v20++];
	} while (v20 < 5);

	return 0;
}

int getBaseDay(int month, int year) {
	if (month == 2) {
		if (year % 4 == 0) {
			if (year % 100 == 0 && year % 400 != 0) {
				return 29;
			}
			else {
				return 28;
			}
		}
		else {
			return 28;
		}
	}
	else if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
		return 31;
	}
	else {
		return 30;
	}
}

std::string getRandomDate() {
	
	struct tm x_years;
	int how_many_years = 1000;
	int randomYear = ( (rand() % 2) == 0 ? -1 : +1 ) * ((rand() % how_many_years) + 1);
	int randomMonth = (rand() % 12) + 1;
	int randomDays = (rand() % getBaseDay(randomMonth, randomYear)) + 1;

	x_years.tm_hour = 0;
	x_years.tm_min = 0;
	x_years.tm_sec = 0;
	x_years.tm_year = 2018 + randomYear;
	x_years.tm_mon = (1 - randomMonth) <= 0 ? 1 + (12 - randomMonth) : 1 - randomMonth;
	x_years.tm_mday = (1 - randomDays) <= 0 ? 1 + (getBaseDay(x_years.tm_mon, x_years.tm_year) - randomDays) : 1 - randomDays;

	std::string result = std::to_string(x_years.tm_year) + "-" + std::to_string(x_years.tm_mon) + "-" + std::to_string(x_years.tm_mday);
	
	return result;
}

Generator::Generator()
{
	int pos = 0;

	int seed = 548762159;

	std::set<std::string> data;

	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);

	std::ofstream f1000;
	std::ofstream f5000;
	std::ofstream f10000;
	std::ofstream f50000;
	std::ofstream f100000;
	std::ofstream f500000;
	std::ofstream f1000000;
	std::string path = "C:\\Users\\mattia\\Development\\UMU\\DGA-DATA\\data\\locky\\list\\";

	f1000.open(path + "1000.txt");
	f5000.open(path + "5000.txt");
	f10000.open(path + "10000.txt");
	f50000.open(path + "50000.txt");
	f100000.open(path + "100000.txt");
	f500000.open(path + "500000.txt");
	f1000000.open(path + "1000000.txt");

	bool stop = false;
	int forceCloseCounter = 0;
	int counter = 0;

	while (!stop)
	{
		char charsdomain[40];
		memset(charsdomain, 0, sizeof(charsdomain));
		if (pos % 100 == 0) {
			// reset the date
			const char *date = getRandomDate().c_str();
			char buf[5];
			strncpy_s(buf, 5, date, 4);
			if (atoi(buf) != 0) {
				systemTime.wYear = atoi(buf);
			}
			memset(buf, 0, sizeof(buf));
			strncpy_s(buf, 5, date + 5, 2);
			if (atoi(buf) != 0)
			{
				systemTime.wMonth = atoi(buf);
			}
			memset(buf, 0, sizeof(buf));
			strncpy_s(buf, 5, date + 8, 2);
			if (atoi(buf) != 0)
			{
				systemTime.wDay = atoi(buf);
			}
		}

		LockyDGA(charsdomain, pos++, seed, systemTime);

		std::string domain = charsdomain;

		int datasize = data.size();
		data.insert(domain);

		//printf("DGA %d = %s\n", pos-1, domain);
		// If it's a collision ignore it.
		if (data.size() == datasize) {
			forceCloseCounter++;
			if (forceCloseCounter == 10 * counter) {
				f1000.close();
				f5000.close();
				f10000.close();
				f50000.close();
				f100000.close();
				f500000.close();
				f1000000.close();
				stop = true;
			}

			if (forceCloseCounter % 100 == 0) {
				seed = rand();
				srand(seed);
			}

			continue;
		}

		counter = counter + 1;

		if (data.size() <= 1000) {
			f1000 << domain << "\n";
			f5000 << domain << "\n";
			f10000 << domain << "\n";
			f50000 << domain << "\n";
			f100000 << domain << "\n";
			f500000 << domain << "\n";
			f1000000 << domain << "\n";
		}
		else if (data.size() <= 5000) {
			f5000 << domain << "\n";
			f10000 << domain << "\n";
			f50000 << domain << "\n";
			f100000 << domain << "\n";
			f500000 << domain << "\n";
			f1000000 << domain << "\n";
		}
		else if (data.size() <= 10000) {
			f10000 << domain << "\n";
			f50000 << domain << "\n";
			f100000 << domain << "\n";
			f500000 << domain << "\n";
			f1000000 << domain << "\n";
		}
		else if (data.size() <= 50000) {
			f50000 << domain << "\n";
			f100000 << domain << "\n";
			f500000 << domain << "\n";
			f1000000 << domain << "\n";
		}
		else if (data.size() <= 100000) {
			f100000 << domain << "\n";
			f500000 << domain << "\n";
			f1000000 << domain << "\n";
		}
		else if (data.size() <= 500000) {
			f500000 << domain << "\n";
			f1000000 << domain << "\n";
		}
		else if (data.size() <= 1000000) {
			f1000000 << domain << "\n";
		}
		else {
			f1000.close();
			f5000.close();
			f10000.close();
			f50000.close();
			f100000.close();
			f500000.close();
			f1000000.close();
			stop = true;
			break;
		}

	}

}



Generator::~Generator()
{
}

int main(int argc, char* argv[]) {
	Generator x = Generator();

	return 0;
}
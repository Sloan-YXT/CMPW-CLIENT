const char *config_teml = "application livevideo%d{\n"
						  "	live on;\n"
						  "	#hls on;\n"
						  "	#hls_path /home/yaoxuetao/software/nginx-rtmp-module-master/hls;\n"
						  "}\n";
#define TOTAL 10
#include "iostream"
#include "fstream"
#include "sstream"
using namespace std;
int main(void)
{
	ofstream res_file("res.txt", ios_base::out | ios_base::trunc);

	for (int i = 0; i < TOTAL; i++)
	{
		char buf[200];
		sprintf(buf, config_teml, i);
		res_file << buf;
	}
	res_file.close();
}
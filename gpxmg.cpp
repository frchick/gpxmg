// gpxmg.cpp : 
//

#include <stdlib.h>
#include <wchar.h>
#include <algorithm>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"

namespace rx = rapidxml;

//-----------------------------------------------------------------------------
class WPT
{
public:
	WPT(const char *lat, const char *lon, const char *name, const char *ele, const char *time, const char *cmt) :
		m_lat(lat),
		m_lon(lon),
		m_name(name),
		m_ele(ele),
		m_time(time),
		m_cmt(cmt)
	{
	}
	WPT(const WPT &wpt) :
		m_lat(wpt.m_lat),
		m_lon(wpt.m_lon),
		m_name(wpt.m_name),
		m_ele(wpt.m_ele),
		m_time(wpt.m_time),
		m_cmt(wpt.m_cmt)
	{
	}

	void setNoChange()
	{
		m_noChange = true;
	}

	bool getNoChange() const
	{
		return m_noChange;
	}

	bool operator<(const WPT& a) const noexcept
	{
		const int c0 = strcmp(m_lat, a.m_lat);
		if(c0 != 0) return (c0 < 0);
		const int c1 = strcmp(m_lon, a.m_lon);
		return (c1 < 0);
	}
	bool operator==(const WPT& a) const noexcept
	{
		const int c0 = strcmp(m_lat, a.m_lat);
		if(c0 != 0) return false;
		const int c1 = strcmp(m_lon, a.m_lon);
		return (c1 == 0);
	}
	bool isSameName(const WPT& a) const noexcept
	{
		return (strcmp(m_name, a.m_name) == 0);
	}

	void print() const
	{
		printf("[%s, %s] %s, %s, %s, %s\n", m_lat, m_lon, m_ele, m_time, m_name, m_cmt);
	}

	void outputWpt(FILE *file)
	{
		fprintf(file,
			" <wpt lat=\"%s\" lon=\"%s\" iswarning=\"0\">\n"
			"  <ele>%s</ele>\n"
			"  <time>%s</time>\n"
			"  <name>%s</name>\n"
			"  <cmt>%s</cmt>\n"
			" </wpt>\n",
			m_lat, m_lon,
			m_ele,
			m_time,
			m_name,
			m_cmt);
	}

protected:
	const char *m_lat = nullptr;
	const char *m_lon = nullptr;
	const char *m_name = nullptr;
	const char *m_ele = nullptr;
	const char *m_time = nullptr;
	const char *m_cmt = nullptr;
	// 変更が無いことフラグ
	bool m_noChange = false;
};

//-----------------------------------------------------------------------------
bool readGpx(rx::xml_document<> &doc, rx::file<> &file, std::vector<WPT> &wpts)
{
	doc.parse<rx::parse_trim_whitespace>(file.data());

	rx::xml_node<>* gpx = doc.first_node("gpx");
	if(gpx == nullptr) return false;

	rx::xml_node<>* wpt = gpx->first_node("wpt");
	while(wpt)
	{
		rx::xml_attribute<> *lat = wpt->first_attribute("lat");
		rx::xml_attribute<> *lon = wpt->first_attribute("lon");
		rx::xml_node<> *name = wpt->first_node("name");
		rx::xml_node<> *ele = wpt->first_node("ele");
		rx::xml_node<> *time = wpt->first_node("time");
		rx::xml_node<> *cmt = wpt->first_node("cmt");
		if(lat && lon){
			wpts.emplace_back(
				lat->value(), lon->value(),
				(name? name->value(): ""),
				(ele? ele->value(): ""),
				(time? time->value(): ""),
				(cmt? cmt->value(): ""));
		}
		wpt = wpt->next_sibling("wpt");
	}

	return true;
}

//-----------------------------------------------------------------------------
int main(int argc, const char *argv[])
{
	if(argc < 4) return 0;

	try
	{
		// 読み込み
		rx::file<> file0(argv[1]);
		rx::file<> file1(argv[2]);
		rx::file<> *files[] = { &file0, &file1 };

		rx::xml_document<> docs[2];
		std::vector<WPT> wpts[2];

		for(int i = 0; i < 2; i++)
		{
			if(!readGpx(docs[i], *files[i], wpts[i]))
			{
				printf(">エラー: gpxの読み込み失敗。\"%s\"\n", argv[1+i]);
				return -1;
			}
			printf(">入力ファイル: %s\n", argv[1+i]);
		}

		// 結合してファイルに出力
		FILE *file = nullptr;
		if(fopen_s(&file, argv[3], "wt") != 0)
		{
			printf(">エラー: ファイルの作成失敗。\"%s\"\n", argv[3]);
			return -1;
		}

		fprintf(file,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		fprintf(file,
			"<gpx version=\"1.1\" "
			"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
			"xmlns=\"http://www.topografix.com/GPX/1/1\" "
			"xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n");

		for(int i = 0; i < 2; i++)
		{
			for(auto it = wpts[i].begin(); it != wpts[i].end(); it++)
			{
				it->outputWpt(file);
			}
		}
		
		fprintf(file,
			"</gpx>\n");
		fclose(file);

		printf(">差分ファイル: %s\n", argv[3]);
	}
	catch(const std::runtime_error& e)
	{
		printf(">エラー: %s\n", e.what());
		return -1;
	}
	catch (const rx::parse_error& e)
	{
		printf(">エラー: %s [%s]\n", e.what(), e.where<char>());
		return -1;
	}

	return 0;
}

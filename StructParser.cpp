// StructParser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <fstream>
#include <string>
#include <chrono>
#include <Windows.h>
#include <sstream>
#include <algorithm>

#undef max

std::string GetClipboardText()
{
	// Try opening the clipboard
	if (!OpenClipboard(nullptr))
		return std::string();

	HANDLE hData = GetClipboardData(CF_TEXT);
	char* pszText = static_cast<char*>(GlobalLock(hData));
	if (pszText == nullptr)
		return std::string();

	GlobalUnlock(hData);

	// Release the clipboard
	CloseClipboard();

	return pszText;
}

// #define GENERATE_CSV

static const std::string html_header = {"<!DOCTYPE html><html><head><style>\
table, th, td {border: 1px solid black; border-collapse: collapse;}\
th, td {padding: 18px;text-align: left;}\
table#t01 tr:nth-child(even) {background-color: #eee;}\
table#t01 tr:nth-child(odd) {background-color: #fff;}\
table#t01 th {background-color: lightgreen;color: black;}\
</style></head><body><table id=\"t01\"><tr><th>Struct Offset</th><th>Size</th><th>Type</th><th>Name in C</th><th>Description</th></tr>"};

static const std::map < std::string, uint8_t> types =
{
	{"uint8_t", sizeof(uint8_t), },
	{"int8_t", sizeof(int8_t)},
	{"uint16_t", sizeof(uint16_t)},
	{"int16_t", sizeof(int16_t)},
	{"uint32_t", sizeof(uint32_t)},
	{"int32_t", sizeof(int32_t)},
	{"uint64_t", sizeof(uint64_t)},
	{"int64_t", sizeof(int64_t)},
	{"float", sizeof(float)},
	{"double", sizeof(double)},
};

class CStructElement
{
public:
	CStructElement(size_t &prev_offset, const char* var_type, const char* var_name, size_t &asize, const char* comment) :
		name(var_name), desc(comment), array_size(asize)
	{
		auto f = types.find(var_type);
		if (f != types.end())
		{
			type_size = f->second;
			str_type = f->first;
			offset = prev_offset;
			if (asize == std::numeric_limits<size_t>::max())
				prev_offset += f->second;
			else
				prev_offset += f->second * asize;
		}
		else
		{
			throw 1;
		}
	}
	uint8_t type_size;
	std::string str_type;
	size_t offset;  /**< starting offset of this variable */
	size_t array_size;  /**< std::numeric_limits<size_t>::max() if isn't array */
	std::string name; /**< variable name */
	std::string desc;  /**< documentation */
	std::string parent; /**< parent in case of child structure */
};

class CStructure
{
	std::string name;
	std::vector<CStructElement*> members;
public:
	CStructure(void) : curr_pointer(0) { }
	CStructure(std::string strName)
		:name(strName), curr_pointer(0) { }
	CStructure(const char* strName)
		:name(strName), curr_pointer(0) { }

	void AddMember(CStructElement* st) { if(st) members.push_back(st); }
	void SetName(const char* st_name) { name = std::string(st_name); }
	const std::string& GetName(void) { return name; }
	std::vector<CStructElement*>& GetMembers(void) { return members; }
	size_t curr_pointer;
};

static uint8_t in_union = 0, union_first_parsed = 0;
static uint8_t struct_found, brace_found = 0, embedded_struct = 0;
static int emb_struct_size = 0;
static CStructure* st = NULL;

int ParseElement(std::list<std::string>::iterator& it)
{
	char type[128], name[128], comment[128];
	memset(type, 0, sizeof(type));
	memset(name, 0, sizeof(name));
	memset(comment, 0, sizeof(comment));
	size_t asize = std::numeric_limits<size_t>::max();
	int ret = sscanf(it->c_str(), "%s %127[^;];  /**< %127[^*]*/", type, name, comment);
	int array_ret = sscanf(name, "%*96[^[][%d]", &asize);
	if (ret >= 2)
	{
		union_first_parsed = 1;
		CStructElement* e = NULL;
		try
		{
			e = new CStructElement(st->curr_pointer, type, name, asize, comment);
			st->AddMember(e);
		}
		catch (...)
		{
			printf("Error: Invalid line: %s\n", it->c_str());
		}
	}
	else
	{
		// printf("Warning: Unknown item (line: %d): %s\n", std::distance(lines.begin(), it), it->c_str());
	}
	return 0;
}

int main()
{
	static_assert(sizeof(uint8_t) == 1 && sizeof(int8_t) == 1, "Invalid uint8_t or int8_t size");
	static_assert(sizeof(uint16_t) == 2 && sizeof(int16_t) == 2, "Invalid uint16_t or int16_t size");
	static_assert(sizeof(uint32_t) == 4 && sizeof(int32_t) == 4, "Invalid uint32_t or int32_t size");
	static_assert(sizeof(uint64_t) == 8 && sizeof(int64_t) == 8 && sizeof(double) == 8, "Invalid uint64_t, int64_t or double size");


	std::string str2 = GetClipboardText();

	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::ifstream in("input.txt");
	if (!in.is_open())
	{
		printf("Failed to open input.txt file");
		system("puase");
	}
#if 0
	std::istringstream in(str2);
#endif
	std::string str;
	std::list < std::string> lines;
	while (std::getline(in, str))
	{
		if (str.size() > 0)
		{
			str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
			str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
			lines.push_back(str);
		}
	}
	std::vector<CStructure*> strucutres;
	std::list<std::string>::iterator it = lines.begin();
	while (it != lines.end())
	{
		if (((it)->at(0) == '/' && (it)->at(1) == '/') || (it)->at(0) == '#')  /* Skip line comments and preprocessor defines */
		{
			++it;
			continue;
		}

		if (in_union)
		{
			if ((it)->find("};") != std::string::npos)
			{
				in_union--;
				++it;
				continue;
			}
		}
		if ((it)->find("typedef struct") != std::string::npos)
		{
			struct_found = 1;
			if ((it)->find("{") != std::string::npos)
			{
				brace_found = 1;
				st = new CStructure;
			}
			++it;
			continue;
		}		
		if (*(it) == "{" && struct_found && !brace_found)
		{
			brace_found = 1;
			st = new CStructure;
			++it;
			continue;
		}
		if ((it)->find("union{") != std::string::npos)
		{
			in_union = 2;
			union_first_parsed = 0;
		}
		Again:
		int current_run = 0;
		if (it->find("struct") != std::string::npos && struct_found && brace_found && !in_union)
		{
			++it;
			if (it->find("{") != std::string::npos)
				++it;

			auto start = it;
			auto end_struct = it;
			auto end_struct2 = it;
			while (it != lines.end())
			{
				if (it->find("}") != std::string::npos)
				{
					if (*it == "};")
					{

					}
					else
					{
						char struct_name[128];
						int array_ret = sscanf(it->c_str(), "%96[^[][%d]", struct_name , &emb_struct_size);
						if (array_ret < 2)
						{
							array_ret = sscanf(it->c_str(), "%96[^;]", struct_name);
							if (array_ret == 1 && struct_name)
								emb_struct_size = 1;
						}
						if (emb_struct_size)
						{
							printf("embedded structure %s[%d]", struct_name, emb_struct_size);
						}
					}
					end_struct = it++;
					break;
				}
				++it;
			}

		Ide:
			it = start;
			while (it != end_struct)
			{
				ParseElement(it);

				if (it->find("}") != std::string::npos)
				{
					++it;
					break;
				}
				++it;
			}

			while (++current_run < emb_struct_size)
				goto Ide;
			++it;
			goto Again;
		}
		if ((it)->at(0) == '}')
		{
			char struct_name[128];
			memset(struct_name, 0, sizeof(struct_name));
			int ret = sscanf(it->c_str(), "} %127[^;];", struct_name);
			st->SetName(struct_name);
			strucutres.push_back(st);
			st = nullptr;
			struct_found = brace_found = in_union = union_first_parsed = emb_struct_size = current_run = 0;
			++it;
			continue;
		}

		if (st)
		{
			if (in_union && union_first_parsed)
			{
				++it;
				continue;
			}
			ParseElement(it);
		}
		++it;
	}

	system("mkdir out");
	for (auto &s : strucutres)
	{
		std::ofstream out;
#ifdef GENERATE_CSV
		out.open("out/" + std::string(s->GetName() + ".csv"), std::ofstream::out);
		out << "offset,size,type,varname,description" << std::endl;
#else
		out.open("out/" + std::string(s->GetName() + ".html"), std::ofstream::out);
		out << html_header;
		out << "<tr><th>Size: " << s->curr_pointer << " </th><th></th><th></th><th></th><th></th></tr>" << std::endl;
#endif
		for (auto &&m : s->GetMembers())
		{
			char str[256];
			size_t len = m->type_size * ((m->array_size == std::numeric_limits<size_t>::max()) ? 1 : m->array_size);
#ifdef GENERATE_CSV
			if(len == 1)	
				snprintf(str, sizeof(str), "%d,%d,%s,%s,%s\n", m->offset, len, m->str_type.c_str(), m->name.c_str(), m->desc.c_str());
			else
				snprintf(str, sizeof(str), "%d - %d,%d,%s,%s,%s\n", m->offset, (m->offset + len) - 1, len, m->str_type.c_str(), m->name.c_str(), m->desc.c_str());
#else
			if (len == 1)
				snprintf(str, sizeof(str), "<tr><td>%d</td><td>%d</td><td>%s</td><td>%s</td><td>%s</td></tr>", m->offset, len, m->str_type.c_str(), m->name.c_str(), m->desc.c_str());
			else
				snprintf(str, sizeof(str), "<tr><td>%d - %d</td><td>%d</td><td>%s</td><td>%s</td><td>%s</td></tr>", m->offset, (m->offset + len) - 1, len, m->str_type.c_str(), m->name.c_str(), m->desc.c_str());
#endif
			out << str << std::endl;
		}
#ifdef GENERATE_CSV
		out << "size: " << s->curr_pointer << std::endl;
#else
		out << "</table></body></html>" << std::endl;;
#endif
		out.close();
	}
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	int64_t dif = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
	printf("\n%d structure has been exported in %.6lf ms\n", strucutres.size(), (double)((double)dif / 1000000.0));
	system("pause");
}

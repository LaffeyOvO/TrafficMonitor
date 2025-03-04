﻿#include "stdafx.h"
#include "IniHelper.h"
#include "TrafficMonitor.h"


CIniHelper::CIniHelper(const wstring& file_path)
{
    m_file_path = file_path;
    ifstream file_stream{ file_path };
    if (!file_stream.is_open())
        return;
    // 获取文件大小
    file_stream.seekg(0, std::ios::end);
    size_t file_size = static_cast<size_t>(file_stream.tellg());
    file_stream.seekg(0, std::ios::beg);
    // 读取文件内容
    string ini_str;
    ini_str.resize(file_size + 1);
    file_stream.read(&ini_str[0], file_size);
    // 检查并添加末尾的空行
    if (!ini_str.empty() && ini_str.back() != L'\n')
        ini_str.push_back(L'\n');
    //判断文件是否是utf8编码
    bool is_utf8;
    if (ini_str.size() >= 3 && ini_str[0] == -17 && ini_str[1] == -69 && ini_str[2] == -65)
    {
        //如果有UTF8的BOM，则删除BOM
        is_utf8 = true;
        ini_str = ini_str.substr(3);
    }
    else
    {
        is_utf8 = false;
    }
    //转换成Unicode
    m_ini_str = CCommon::StrToUnicode(ini_str.c_str(), is_utf8);
}

CIniHelper::CIniHelper(UINT id, bool is_utf8)
{
    m_ini_str = CCommon::GetTextResource(id, is_utf8 ? 1 : 0);
}

CIniHelper::CIniHelper()
{
}


CIniHelper::~CIniHelper()
{
}

void CIniHelper::FromDirectString(const wstring& str_content)
{
    m_ini_str = str_content;
}

void CIniHelper::SetSaveAsUTF8(bool utf8)
{
    m_save_as_utf8 = utf8;
}

void CIniHelper::WriteString(const wchar_t * AppName, const wchar_t * KeyName, const wstring& str)
{
    wstring write_str{ str };
    if (!write_str.empty() && (write_str[0] == L' ' || write_str.back() == L' '))       //如果字符串前后含有空格，则在字符串前后添加引号
    {
        write_str = DEF_CH + write_str;
        write_str.push_back(DEF_CH);
    }
    _WriteString(AppName, KeyName, write_str);
}

wstring CIniHelper::GetString(const wchar_t * AppName, const wchar_t * KeyName, const wchar_t* default_str) const
{
    wstring rtn{_GetString(AppName, KeyName, default_str)};
    //如果读取的字符串前后有指定的字符，则删除它
    if (!rtn.empty() && (rtn.front() == L'$' || rtn.front() == DEF_CH))
        rtn = rtn.substr(1);
    if (!rtn.empty() && (rtn.back() == L'$' || rtn.back() == DEF_CH))
        rtn.pop_back();
    return rtn;
}

void CIniHelper::WriteInt(const wchar_t * AppName, const wchar_t * KeyName, int value)
{
    _WriteString(AppName, KeyName, std::to_wstring(value));
}

int CIniHelper::GetInt(const wchar_t * AppName, const wchar_t * KeyName, int default_value) const
{
    wstring rtn{ _GetString(AppName, KeyName, std::to_wstring(default_value).c_str()) };
    return _ttoi(rtn.c_str());
}

void CIniHelper::WriteBool(const wchar_t * AppName, const wchar_t * KeyName, bool value)
{
    if(value)
        _WriteString(AppName, KeyName, wstring(L"true"));
    else
        _WriteString(AppName, KeyName, wstring(L"false"));
}

bool CIniHelper::GetBool(const wchar_t * AppName, const wchar_t * KeyName, bool default_value) const
{
    wstring rtn{ _GetString(AppName, KeyName, (default_value ? L"true" : L"false")) };
    if (rtn == L"true")
        return true;
    else if (rtn == L"false")
        return false;
    else
        return (_ttoi(rtn.c_str()) != 0);
}

void CIniHelper::WriteIntArray(const wchar_t * AppName, const wchar_t * KeyName, const int * values, int size)
{
    CString str, tmp;
    for (int i{}; i < size; i++)
    {
        tmp.Format(_T("%d,"), values[i]);
        str += tmp;
    }
    _WriteString(AppName, KeyName, wstring(str));
}

void CIniHelper::GetIntArray(const wchar_t * AppName, const wchar_t * KeyName, int * values, int size, int default_value) const
{
    CString default_str;
    default_str.Format(_T("%d"), default_value);
    wstring str;
    str = _GetString(AppName, KeyName, default_str);
    std::vector<wstring> split_result;
    CCommon::StringSplit(str, L',', split_result);
    for (int i = 0; i < size; i++)
    {
        if (i < split_result.size())
            values[i] = _wtoi(split_result[i].c_str());
        else if (i > 0)
            values[i] = values[i - 1];
        else
            values[i] = default_value;
    }
}

void CIniHelper::WriteBoolArray(const wchar_t * AppName, const wchar_t * KeyName, const bool * values, int size)
{
    int value{};
    for (int i{}; i < size; i++)
    {
        if (values[i])
            value |= (1 << i);
    }
    return WriteInt(AppName, KeyName, value);
}

void CIniHelper::GetBoolArray(const wchar_t * AppName, const wchar_t * KeyName, bool * values, int size, bool default_value) const
{
    int value = GetInt(AppName, KeyName, 0);
    for (int i{}; i < size; i++)
    {
        values[i] = ((value >> i) % 2 != 0);
    }
}

void CIniHelper::WriteStringList(const wchar_t* AppName, const wchar_t* KeyName, const vector<wstring>& values)
{
    wstring str_write = MergeStringList(values);
    _WriteString(AppName, KeyName, str_write);
}

void CIniHelper::GetStringList(const wchar_t* AppName, const wchar_t* KeyName, vector<wstring>& values, const vector<wstring>& default_value) const
{
    wstring default_str = MergeStringList(default_value);
    wstring str_value = _GetString(AppName, KeyName, default_str.c_str());
    SplitStringList(values, str_value);
}

void CIniHelper::SaveFontData(const wchar_t * AppName, const FontInfo & font)
{
    WriteString(AppName, L"font_name", wstring(font.name));
    WriteInt(AppName, L"font_size", font.size);
    bool style[4];
    style[0] = font.bold;
    style[1] = font.italic;
    style[2] = font.underline;
    style[3] = font.strike_out;
    WriteBoolArray(AppName, L"font_style", style, 4);
}

vector<wstring> CIniHelper::GetAllAppName(const wstring& prefix) const
{
    vector<wstring> list;
    size_t pos{};
    while ((pos = m_ini_str.find(L"\n[" + prefix, pos)) != wstring::npos)
    {
        size_t end = m_ini_str.find(L']', pos + 1);
        if (end != wstring::npos)
        {
            wstring tmp(m_ini_str.begin() + pos + prefix.size() + 2, m_ini_str.begin() + end);
            list.push_back(std::move(tmp));
            pos = end + 1;
        }
    }
    return list;
}

void CIniHelper::GetAllKeyValues(const wstring& AppName, std::map<wstring, wstring>& map) const
{
    wstring app_str{ L"[" };
    app_str.append(AppName).append(L"]");
    size_t app_pos{}, app_end_pos{};
    app_pos = m_ini_str.find(app_str);
    if (app_pos == wstring::npos)
        return;
    app_end_pos = m_ini_str.find(L"\n[", app_pos + 2);
    if (app_end_pos != wstring::npos)
        app_end_pos++;
    app_str = m_ini_str.substr(app_pos, app_end_pos - app_pos);
    vector<wstring> line;
    CCommon::StringSplit(app_str, L'\n', line);
    for (wstring str : line)
    {
        // CCommon::StringSplit会跳过空字符串，str一定非空
        if (str[0] == L';' || str[0] == L'#')   // 跳过注释行（只支持行首注释）
            continue;
        size_t pos = str.find_first_of(L'=');
        if (pos == wstring::npos)
            continue;
        wstring key{ str.substr(0, pos) };
        wstring value{ str.substr(pos + 1) };
        CCommon::StringNormalize(key);
        CCommon::StringNormalize(value);
        if (!key.empty() && !value.empty())
        {
            if (value.front() == L'\"' && value.back() == L'\"')
                value = value.substr(1, value.size() - 2);
            UnEscapeString(value);
            map[key] = value;
        }
    }
}

bool CIniHelper::Save()
{
    if (m_file_path.empty())
        return false;
    ofstream file_stream{ m_file_path };
    if(file_stream.fail())
        return false;
    string ini_str{ CCommon::UnicodeToStr(m_ini_str.c_str(), m_save_as_utf8) };
    if (m_save_as_utf8)     //如果以UTF8编码保存，先插入BOM
    {
        string utf8_bom;
        utf8_bom.push_back(-17);
        utf8_bom.push_back(-69);
        utf8_bom.push_back(-65);
        file_stream << utf8_bom;
    }

    file_stream << ini_str;
    return true;
}

void CIniHelper::LoadFontData(const wchar_t * AppName, FontInfo & font, const FontInfo& default_font) const
{
    font.name = GetString(AppName, L"font_name", default_font.name).c_str();
    font.size = GetInt(AppName, L"font_size", default_font.size);
    bool style[4];
    GetBoolArray(AppName, L"font_style", style, 4);
    font.bold = style[0];
    font.italic = style[1];
    font.underline = style[2];
    font.strike_out = style[3];
}

void CIniHelper::LoadMainWndColors(const wchar_t * AppName, const wchar_t * KeyName, std::map<CommonDisplayItem, COLORREF>& text_colors, COLORREF default_color)
{
    CString default_str;
    default_str.Format(_T("%d"), default_color);
    wstring str;
    str = _GetString(AppName, KeyName, default_str);
    std::vector<wstring> split_result;
    CCommon::StringSplit(str, L',', split_result);
    size_t index = 0;
    for (auto iter = theApp.m_plugins.AllDisplayItemsWithPlugins().begin(); iter != theApp.m_plugins.AllDisplayItemsWithPlugins().end(); ++iter)
    {
        if (index < split_result.size())
            text_colors[*iter] = _wtoi(split_result[index].c_str());
        else if (!split_result.empty())
            text_colors[*iter] = _wtoi(split_result[0].c_str());
        else
            text_colors[*iter] = default_color;
        index++;
    }
}

void CIniHelper::SaveMainWndColors(const wchar_t * AppName, const wchar_t * KeyName, const std::map<CommonDisplayItem, COLORREF>& text_colors)
{
    CString str;
    for (auto iter = text_colors.begin(); iter != text_colors.end(); ++iter)
    {
        CString tmp;
        tmp.Format(_T("%d,"), iter->second);
        str += tmp;
    }
    _WriteString(AppName, KeyName, wstring(str));

}

void CIniHelper::LoadTaskbarWndColors(const wchar_t* AppName, const wchar_t* KeyName, std::map<CommonDisplayItem, TaskbarItemColor>& text_colors, const wchar_t* default_str)
{
    wstring str;
    str = _GetString(AppName, KeyName, default_str);
    std::vector<wstring> split_result;
    CCommon::StringSplit(str, L',', split_result);
    size_t index = 0;
    COLORREF default_color = _wtoi(default_str);
    for (auto iter = theApp.m_plugins.AllDisplayItemsWithPlugins().begin(); iter != theApp.m_plugins.AllDisplayItemsWithPlugins().end(); ++iter)
    {
        if (index < split_result.size())
            text_colors[*iter].label = _wtoi(split_result[index].c_str());
        else if (!split_result.empty())
            text_colors[*iter].label = _wtoi(split_result[0].c_str());
        else
            text_colors[*iter].label = default_color;

        if (index + 1 < split_result.size())
            text_colors[*iter].value = _wtoi(split_result[index + 1].c_str());
        else if (split_result.size() > 1)
            text_colors[*iter].value = _wtoi(split_result[1].c_str());
        else
            text_colors[*iter].value = default_color;
        index += 2;
    }

}

void CIniHelper::LoadTaskbarWndColors(const wchar_t * AppName, const wchar_t * KeyName, std::map<CommonDisplayItem, TaskbarItemColor>& text_colors, COLORREF default_color)
{
    CString default_str;
    default_str.Format(_T("%d"), default_color);
    LoadTaskbarWndColors(AppName, KeyName, text_colors, default_str.GetString());
}

void CIniHelper::SaveTaskbarWndColors(const wchar_t * AppName, const wchar_t * KeyName, const std::map<CommonDisplayItem, TaskbarItemColor>& text_colors)
{
    CString str;
    for (auto iter = text_colors.begin(); iter != text_colors.end(); ++iter)
    {
        CString tmp;
        tmp.Format(_T("%d,%d,"), iter->second.label, iter->second.value);
        str += tmp;
    }
    _WriteString(AppName, KeyName, wstring(str));
}

void CIniHelper::LoadDisplayStr(const wchar_t* AppName, DispStrings& disp_str, bool is_main_window) const
{
    disp_str.Get(TDI_UP) = GetString(AppName, L"up_string", is_main_window ? CCommon::LoadText(IDS_UPLOAD_DISP, L": $").GetString() : L"↑: $");
    disp_str.Get(TDI_DOWN) = GetString(AppName, L"down_string", is_main_window ? CCommon::LoadText(IDS_DOWNLOAD_DISP, L": $").GetString() : L"↓: $");
    disp_str.Get(TDI_TOTAL_SPEED) = GetString(AppName, L"total_speed_string", _T("↑↓: $"));
    disp_str.Get(TDI_CPU) = GetString(AppName, L"cpu_string", L"CPU: $");
    disp_str.Get(TDI_CPU_FREQ) = GetString(AppName, L"cpu_freq_string", CCommon::LoadText(IDS_CPU_FREQ, _T(": $")));
    disp_str.Get(TDI_MEMORY) = GetString(AppName, L"memory_string", CCommon::LoadText(IDS_MEMORY_DISP, _T(": $")));
    disp_str.Get(TDI_GPU_USAGE) = GetString(AppName, L"gpu_string", CCommon::LoadText(IDS_GPU_DISP, _T(": $")));
    disp_str.Get(TDI_CPU_TEMP) = GetString(AppName, L"cpu_temp_string", L"CPU: $");
    disp_str.Get(TDI_GPU_TEMP) = GetString(AppName, L"gpu_temp_string", CCommon::LoadText(IDS_GPU_DISP, _T(": $")));
    disp_str.Get(TDI_HDD_TEMP) = GetString(AppName, L"hdd_temp_string", CCommon::LoadText(IDS_HDD_DISP, _T(": $")));
    disp_str.Get(TDI_MAIN_BOARD_TEMP) = GetString(AppName, L"main_board_temp_string", CCommon::LoadText(IDS_MAINBOARD_DISP, _T(": $")));
    disp_str.Get(TDI_HDD_USAGE) = GetString(AppName, L"hdd_string", CCommon::LoadText(IDS_HDD_DISP, _T(": $")));
    disp_str.Get(TDI_TODAY_TRAFFIC) = GetString(AppName, L"today_traffic_string", CCommon::LoadText(IDS_TRAFFIC_USED, _T(": $")));
}

void CIniHelper::SaveDisplayStr(const wchar_t* AppName, const DispStrings& disp_str)
{
    auto writeDisplayString = [&](const wchar_t* key_name, DisplayItem display_item) {
        if (disp_str.GetAllItems().find(display_item) != disp_str.GetAllItems().end())
        {
            WriteString(AppName, key_name, disp_str.GetConst(display_item));
        }
    };

    writeDisplayString(_T("up_string"), TDI_UP);
    writeDisplayString(_T("down_string"), TDI_DOWN);
    writeDisplayString(_T("total_speed_string"), TDI_TOTAL_SPEED);
    writeDisplayString(_T("cpu_string"), TDI_CPU);
    writeDisplayString(_T("memory_string"), TDI_MEMORY);
    writeDisplayString(_T("gpu_string"), TDI_GPU_USAGE);
    writeDisplayString(_T("cpu_temp_string"), TDI_CPU_TEMP);
    writeDisplayString(_T("cpu_freq_string"), TDI_CPU_FREQ);
    writeDisplayString(_T("gpu_temp_string"), TDI_GPU_TEMP);
    writeDisplayString(_T("hdd_temp_string"), TDI_HDD_TEMP);
    writeDisplayString(_T("main_board_temp_string"), TDI_MAIN_BOARD_TEMP);
    writeDisplayString(_T("hdd_string"), TDI_HDD_USAGE);
    writeDisplayString(_T("today_traffic_string"), TDI_TODAY_TRAFFIC);
}

void CIniHelper::LoadPluginDisplayStr(const wchar_t* AppName, DispStrings& disp_str)
{
    for (const auto& plugin : theApp.m_plugins.GetPluginItems())
    {
        disp_str.Load(plugin->GetItemId(), GetString(AppName, plugin->GetItemId(), plugin->GetItemLableText()));
    }
}

void CIniHelper::SavePluginDisplayStr(const wchar_t* AppName, const DispStrings& disp_str)
{
    for (const auto& plugin : theApp.m_plugins.GetPluginItems())
    {
        if (disp_str.GetAllItems().find(plugin) != disp_str.GetAllItems().end())
            WriteString(AppName, plugin->GetItemId(), disp_str.GetConst(plugin));
    }
}

void CIniHelper::UnEscapeString(wstring& str)
{
    bool escape{ false };
    wstring result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); i++)
    {
        wchar_t ch = str[i];
        if (escape)
        {
            switch (ch)
            {
            case L'n': result += L'\n'; break;
            case L'r': result += L'\r'; break;
            case L't': result += L'\t'; break;
            case L'"': result += L'"'; break;
            case L';': result += L';'; break;
            case L'#': result += L'#'; break;
            case L'\\': result += L'\\'; break;
            default:result += '\\'; result += ch; break;
            }
            escape = false;
        }
        else if (ch == L'\\')
            escape = true;
        else if (i > 0 && ch == '\"' && str[i - 1] == '\"')     //两个连续的引号只保留一个引号
            continue;
        else
            result += ch;
    }
    str.swap(result);
}

void CIniHelper::_WriteString(const wchar_t * AppName, const wchar_t * KeyName, const wstring & str)
{
    wstring app_str{ L"[" };
    app_str.append(AppName).append(L"]");
    size_t app_pos{}, app_end_pos, key_pos;
    app_pos = m_ini_str.find(app_str);
    if (app_pos == wstring::npos)       //找不到AppName，则在最后面添加
    {
        if (!m_ini_str.empty() && m_ini_str.back() != L'\n')
            m_ini_str += L"\n";
        app_pos = m_ini_str.size();
        m_ini_str += app_str;
        m_ini_str += L"\n";
    }
    app_end_pos = m_ini_str.find(L"\n[", app_pos + 2);
    if (app_end_pos != wstring::npos)
        app_end_pos++;

    key_pos = m_ini_str.find(wstring(L"\n") + KeyName + L' ', app_pos);     //查找“\nkey_name ”
    if (key_pos >= app_end_pos)     //如果找不到“\nkey_name ”，则查找“\nkey_name=”
        key_pos = m_ini_str.find(wstring(L"\n") + KeyName + L'=', app_pos);
    if (key_pos >= app_end_pos)             //找不到KeyName，则插入一个
    {
        //wchar_t buff[256];
        //swprintf_s(buff, L"%s = %s\n", KeyName, str.c_str());
        std::wstring str_temp = KeyName;
        str_temp += L" = ";
        str_temp += str;
        str_temp += L"\n";
        if (app_end_pos == wstring::npos)
            m_ini_str += str_temp;
        else
            m_ini_str.insert(app_end_pos, str_temp);
    }
    else    //找到了KeyName，将等号到换行符之间的文本替换
    {
        size_t str_pos;
        str_pos = m_ini_str.find(L'=', key_pos + 2);
        size_t line_end_pos = m_ini_str.find(L'\n', key_pos + 2);
        if (str_pos > line_end_pos) //所在行没有等号，则插入一个等号
        {
            m_ini_str.insert(key_pos + wcslen(KeyName) + 1, L" =");
            str_pos = key_pos + wcslen(KeyName) + 2;
        }
        else
        {
            str_pos++;
        }
        size_t str_end_pos;
        str_end_pos = m_ini_str.find(L"\n", str_pos);
        m_ini_str.replace(str_pos, str_end_pos - str_pos, L" " + str);
    }
}

wstring CIniHelper::_GetString(const wchar_t * AppName, const wchar_t * KeyName, const wchar_t* default_str) const
{
    wstring app_str{ L"[" };
    app_str.append(AppName).append(L"]");
    size_t app_pos{}, app_end_pos, key_pos;
    app_pos = m_ini_str.find(app_str);
    if (app_pos == wstring::npos)       //找不到AppName，返回默认字符串
        return default_str;

    app_end_pos = m_ini_str.find(L"\n[", app_pos + 2);
    if (app_end_pos != wstring::npos)
        app_end_pos++;

    key_pos = m_ini_str.find(wstring(L"\n") + KeyName + L' ', app_pos);     //查找“\nkey_name ”
    if (key_pos >= app_end_pos)     //如果找不到“\nkey_name ”，则查找“\nkey_name=”
        key_pos = m_ini_str.find(wstring(L"\n") + KeyName + L'=', app_pos);
    if (key_pos >= app_end_pos)             //找不到KeyName，返回默认字符串
    {
        return default_str;
    }
    else    //找到了KeyName，获取等号到换行符之间的文本
    {
        size_t str_pos;
        str_pos = m_ini_str.find(L'=', key_pos + 2);
        size_t line_end_pos = m_ini_str.find(L'\n', key_pos + 2);
        if (str_pos > line_end_pos) //所在行没有等号，返回默认字符串
        {
            return default_str;
        }
        else
        {
            str_pos++;
        }
        size_t str_end_pos;
        str_end_pos = m_ini_str.find(L"\n", str_pos);
        //获取文本
        wstring return_str{ m_ini_str.substr(str_pos, str_end_pos - str_pos) };
        //如果前后有空格，则将其删除
        CCommon::StringNormalize(return_str);
        return return_str;
    }
}

wstring CIniHelper::MergeStringList(const vector<wstring>& values)
{
    wstring str_merge;
    int index = 0;
    //在每个字符串前后加上引号，再将它们用逗号连接起来
    for (const wstring& str : values)
    {
        if (index > 0)
            str_merge.push_back(L',');
        str_merge.push_back(L'\"');
        str_merge += str;
        str_merge.push_back(L'\"');
        index++;
    }
    return str_merge;
}

void CIniHelper::SplitStringList(vector<wstring>& values, wstring str_value)
{
    CCommon::StringSplit(str_value, wstring(L"\",\""), values);
    if (!values.empty())
    {
        //结果中第一项前面和最后一项的后面各还有一个引号，将它们删除
        values.front() = values.front().substr(1);
        values.back().pop_back();
    }
}

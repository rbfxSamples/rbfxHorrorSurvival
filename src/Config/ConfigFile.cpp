/**
  @author Justin Miller (carnalis.j at gmail.com)
  @author Thebluefish
  @license The MIT License (MIT)
  @copyright
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "ConfigFile.h"

#include <Urho3D/Container/Str.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>

using namespace Urho3D;

ConfigFile::ConfigFile(Context* context, bool caseSensitive)
    : Resource(context)
    , caseSensitive_(caseSensitive)
{
}

void ConfigFile::RegisterObject(Context* context) { context->RegisterFactory<ConfigFile>(); }

bool ConfigFile::BeginLoad(Deserializer& source)
{
    unsigned dataSize(source.GetSize());
    if (!dataSize && !source.GetName().empty())
    {
        URHO3D_LOGERROR("Zero sized data in " + source.GetName());
        return false;
    }

    configMap_.push_back(ConfigSection());
    ConfigSection* configSection(&configMap_.back());
    while (!source.IsEof())
    {
        ea::string line(source.ReadLine());

        // Parse headers.
        if (line.starts_with("[") && line.ends_with("]"))
        {
            configMap_.push_back(ConfigSection());
            configSection = &configMap_.back();
        }

        configSection->push_back(line);
    }

    return true;
}

bool ConfigFile::Save(Serializer& dest) const
{
    dest.WriteLine("# AUTO-GENERATED");

    ea::hash_map<ea::string, ea::string> processedConfig;

    // Iterate over all sections, printing out the header followed by the properties.
    for (ea::vector<ConfigSection>::const_iterator itr(configMap_.begin()); itr != configMap_.end(); ++itr)
    {
        if (itr->begin() == itr->end())
        {
            continue;
        }

        // Don't print section if there's nothing to print.
        ea::vector<ea::string>::const_iterator section_itr(itr->begin());
        ea::string header(ParseHeader(*section_itr));

        // Don't print header if it's empty.
        if (header != "")
        {
            dest.WriteLine("");
            dest.WriteLine("[" + header + "]");
        }

        dest.WriteLine("");

        for (; section_itr != itr->end(); ++section_itr)
        {
            const ea::string line(ParseComments(*section_itr));

            ea::string property;
            ea::string value;

            ParseProperty(line, property, value);

            if (processedConfig.contains(property))
            {
                continue;
            }
            processedConfig[property] = value;

            if (property != "" && value != "")
            {
                dest.WriteLine(property + "=" + value);
            }
        }

        dest.WriteLine("");
    }

    return true;
}

bool ConfigFile::Save(Serializer& dest, bool smartSave) const
{
    if (!smartSave)
    {
        return Save(dest);
    }

    ea::hash_map<ea::string, bool> wroteLine;
    ea::string activeSection;

    // Iterate over all sections, printing out the header followed by the properties.
    for (ea::vector<ConfigSection>::const_iterator itr(configMap_.begin()); itr != configMap_.end(); ++itr)
    {
        if (itr->begin() == itr->end())
        {
            continue;
        }

        for (ea::vector<ea::string>::const_iterator section_itr(itr->begin()); section_itr != itr->end(); ++section_itr)
        {
            const ea::string line(*section_itr);

            if (wroteLine.contains(activeSection + line))
            {
                continue;
            }

            wroteLine[activeSection + line] = true;

            if (line.empty())
            {
                continue;
            }

            if (section_itr == itr->begin())
            {
                dest.WriteLine("");
                dest.WriteLine("[" + line + "]");
                activeSection = line;
            }
            else
            {
                dest.WriteLine(line);
            }
        }
    }

    return true;
}

bool ConfigFile::FromString(const ea::string& source)
{
    if (source.empty())
    {
        return false;
    }

    MemoryBuffer buffer(source.c_str(), source.length());
    return Load(buffer);
}

bool ConfigFile::Has(const ea::string& section, const ea::string& parameter)
{
    return GetString(section, parameter) != "";
}

const ea::string ConfigFile::GetString(
    const ea::string& section, const ea::string& parameter, const ea::string& defaultValue)
{
    // Find the correct section.
    ConfigSection* configSection(nullptr);
    for (ea::vector<ConfigSection>::iterator itr(configMap_.begin()); itr != configMap_.end(); ++itr)
    {
        if (itr->begin() == itr->end())
        {
            continue;
        }

        ea::string& header(*(itr->begin()));
        header = ParseHeader(header);

        if (caseSensitive_)
        {
            if (section == header)
            {
                configSection = &(*itr);
            }
        }
        else
        {
            if (section.to_lower() == header.to_lower())
            {
                configSection = &(*itr);
            }
        }
    }

    // Section doesn't exist.
    if (!configSection)
    {
        return defaultValue;
    }

    for (ea::vector<ea::string>::const_iterator itr(configSection->begin()); itr != configSection->end(); ++itr)
    {
        ea::string property;
        ea::string value;
        ParseProperty(*itr, property, value);

        if (property == "" || value == "")
        {
            continue;
        }

        if (caseSensitive_)
        {
            if (parameter == property)
            {
                return value;
            }
        }
        else
        {
            if (parameter.to_lower() == property.to_lower())
            {
                return value;
            }
        }
    }

    return defaultValue;
}

const int ConfigFile::GetInt(const ea::string& section, const ea::string& parameter, const int defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToInt(property);
}

const bool ConfigFile::GetBool(const ea::string& section, const ea::string& parameter, const bool defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToBool(property);
}

const float ConfigFile::GetFloat(const ea::string& section, const ea::string& parameter, const float defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToFloat(property);
}

const Vector2 ConfigFile::GetVector2(
    const ea::string& section, const ea::string& parameter, const Vector2& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToVector2(property);
}

const Vector3 ConfigFile::GetVector3(
    const ea::string& section, const ea::string& parameter, const Vector3& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToVector3(property);
}

const Vector4 ConfigFile::GetVector4(
    const ea::string& section, const ea::string& parameter, const Vector4& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToVector4(property);
}

const Quaternion ConfigFile::GetQuaternion(
    const ea::string& section, const ea::string& parameter, const Quaternion& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToQuaternion(property);
}

const Color ConfigFile::GetColor(const ea::string& section, const ea::string& parameter, const Color& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToColor(property);
}

const IntRect ConfigFile::GetIntRect(
    const ea::string& section, const ea::string& parameter, const IntRect& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToIntRect(property);
}

const IntVector2 ConfigFile::GetIntVector2(
    const ea::string& section, const ea::string& parameter, const IntVector2& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToIntVector2(property);
}

const Matrix3 ConfigFile::GetMatrix3(
    const ea::string& section, const ea::string& parameter, const Matrix3& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToMatrix3(property);
}

const Matrix3x4 ConfigFile::GetMatrix3x4(
    const ea::string& section, const ea::string& parameter, const Matrix3x4& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToMatrix3x4(property);
}

const Matrix4 ConfigFile::GetMatrix4(
    const ea::string& section, const ea::string& parameter, const Matrix4& defaultValue)
{
    ea::string property(GetString(section, parameter));

    if (property == "")
    {
        return defaultValue;
    }

    return ToMatrix4(property);
}

void ConfigFile::Set(const ea::string& section, const ea::string& parameter, const ea::string& value)
{
    // Find the correct section.
    ConfigSection* configSection(nullptr);
    for (ea::vector<ConfigSection>::iterator itr(configMap_.begin()); itr != configMap_.end(); ++itr)
    {
        if (itr->begin() == itr->end())
        {
            continue;
        }

        ea::string& header(*(itr->begin()));
        header = ParseHeader(header);

        if (caseSensitive_)
        {
            if (section == header)
            {
                configSection = &(*itr);
            }
        }
        else
        {
            if (section.to_lower() == header.to_lower())
            {
                configSection = &(*itr);
            }
        }
    }

    if (section == "")
    {
        configSection = &(*configMap_.begin());
    }

    // Section doesn't exist.
    if (!configSection)
    {
        ea::string sectionName(section);

        // Format header.
        if (ConfigFile::ParseHeader(sectionName) == sectionName)
        {
            sectionName = "[" + sectionName + "]";
        }

        // Create section.
        configMap_.push_back(ConfigSection());
        configSection = &configMap_.back();

        // Add header and blank line.
        configSection->push_back(sectionName);
        configSection->push_back("");
    }

    ea::string* line(nullptr);
    unsigned separatorPos(0);
    for (ea::vector<ea::string>::iterator itr(configSection->begin()); itr != configSection->end(); ++itr)
    {
        // Find property separator.
        separatorPos = itr->find("=");
        if (separatorPos == ea::string::npos)
        {
            separatorPos = itr->find(":");
        }

        // Not a property.
        if (separatorPos == ea::string::npos)
        {
            continue;
        }

        ea::string workingLine = ParseComments(*itr);

        ea::string oldParameter(workingLine.substr(0, separatorPos).trimmed());
        ea::string oldValue(workingLine.substr(separatorPos + 1).trimmed());

        // Not the correct parameter.
        if (caseSensitive_ ? (oldParameter == parameter) : (oldParameter.to_lower() == parameter.to_lower()))
        {
            // Replace the value.
            itr->replace(itr->find(oldValue, separatorPos), oldValue.length(), value);
            return;
        }
    }

    // Parameter doesn't exist yet.
    // Find a good place to insert the parameter, avoiding lines which are entirely comments or whitespacing.
    int index(configSection->size() - 1);
    for (int i(index); i >= 0; i--)
    {
        if (ParseComments((*configSection)[i]) != "")
        {
            index = i + 1;
            break;
        }
    }
    configSection->insert_at(index, parameter + "=" + value);
}

// Returns header name without bracket.
const ea::string ConfigFile::ParseHeader(ea::string line)
{
    // Only parse comments outside of headers.
    unsigned commentPos(0);

    while (commentPos != ea::string::npos)
    {
        // Find next comment.
        unsigned lastCommentPos(commentPos);
        unsigned commaPos(line.find("//", commentPos));
        unsigned hashPos(line.find("#", commentPos));
        commentPos = (commaPos < hashPos) ? commaPos : hashPos;

        // Header is behind a comment.
        if (line.find("[", lastCommentPos) > commentPos)
        {
            // Stop parsing this line.
            break;
        }

        // Header is before a comment.
        if (line.find("[") < commentPos)
        {
            unsigned startPos(line.find("[") + 1);
            unsigned l1(line.find("]"));
            unsigned length(l1 - startPos);
            line = line.substr(startPos, length);
            break;
        }
    }

    line = line.trimmed();

    return line;
}

const void ConfigFile::ParseProperty(ea::string line, ea::string& property, ea::string& value)
{
    line = ParseComments(line);

    // Find property separator.
    unsigned separatorPos(line.find("="));
    if (separatorPos == ea::string::npos)
    {
        separatorPos = line.find(":");
    }

    // Not a property.
    if (separatorPos == ea::string::npos)
    {
        property = "";
        value = "";
        return;
    }

    property = line.substr(0, separatorPos).trimmed();
    value = line.substr(separatorPos + 1).trimmed();
}

const ea::string ConfigFile::ParseComments(ea::string line)
{
    // Normalize comment tokens.
    line.replace("//", "#");

    // Strip comments.
    unsigned commentPos(line.find("#"));
    if (commentPos != ea::string::npos)
    {
        line = line.substr(0, commentPos);
    }

    return line;
}

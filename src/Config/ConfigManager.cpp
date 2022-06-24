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

#include "ConfigManager.h"
#include "ConfigFile.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>

using namespace Urho3D;

ConfigManager::ConfigManager(Context* context, const ea::string& defaultFileName, bool caseSensitive, bool saveDefaultParameters) :
        Object(context)
        , defaultFileName_(defaultFileName)
        , caseSensitive_(caseSensitive)
        , saveDefaultParameters_(saveDefaultParameters)
{
    Load();
}

void ConfigManager::RegisterObject(Context* context) {
    context->RegisterFactory<ConfigManager>();
}

// Check if value exists.
bool ConfigManager::Has(const ea::string& section, const ea::string& parameter) {
    return Get(section, parameter) != Variant::EMPTY;
}

// Set value.
void ConfigManager::Set(const ea::string& section, const ea::string& parameter, const Variant& value) {
    SettingsMap* sectionMap(GetSection(section, true));

    sectionMap->operator[](parameter) = value;
    SetGlobalVar(parameter, value);
}

// Get value.
const Variant ConfigManager::Get(const ea::string& section, const ea::string& parameter, const Variant& defaultValue) {
    SettingsMap* sectionMap(GetSection(section));

    // Section doesn't exist.
    if (!sectionMap) {
        return defaultValue;
    }

    // Section exists, parameter doesn't exist.
    if (sectionMap->Find(parameter) == sectionMap->End()) {
        return defaultValue;
    }

    // Section exists, parameter exists.
    return (*sectionMap)[parameter];
}

const ea::string ConfigManager::GetString(const ea::string& section, const ea::string& parameter, const ea::string& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_STRING) {
        return value.GetString();
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const int ConfigManager::GetInt(const ea::string& section, const ea::string& parameter, const int defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_INT && value.GetInt() != defaultValue) {
        return value.GetInt();
    }

    if (value.GetType() == VAR_STRING) {
        return ToInt(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const int ConfigManager::GetUInt(const ea::string& section, const ea::string& parameter, const unsigned defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_INT && value.GetUInt() != defaultValue) {
        return value.GetUInt();
    }

    if (value.GetType() == VAR_STRING) {
        return ToUInt(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const bool ConfigManager::GetBool(const ea::string& section, const ea::string& parameter, const bool defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_BOOL) {
        return value.GetBool();
    }

    if (value.GetType() == VAR_STRING) {
        return ToBool(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const float ConfigManager::GetFloat(const ea::string& section, const ea::string& parameter, const float defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_FLOAT) {
        return value.GetFloat();
    }

    if (value.GetType() == VAR_STRING) {
        return ToFloat(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const Vector2 ConfigManager::GetVector2(const ea::string& section, const ea::string& parameter, const Vector2& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_VECTOR2) {
        return value.GetVector2();
    }

    if (value.GetType() == VAR_STRING) {
        return ToVector2(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const Vector3 ConfigManager::GetVector3(const ea::string& section, const ea::string& parameter, const Vector3& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_VECTOR3) {
        return value.GetVector3();
    }

    if (value.GetType() == VAR_STRING) {
        return ToVector3(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const Vector4 ConfigManager::GetVector4(const ea::string& section, const ea::string& parameter, const Vector4& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_VECTOR4) {
        return value.GetVector4();
    }

    if (value.GetType() == VAR_STRING) {
        return ToVector4(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const Quaternion ConfigManager::GetQuaternion(const ea::string& section, const ea::string& parameter, const Quaternion& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_QUATERNION) {
        return value.GetQuaternion();
    }

    if (value.GetType() == VAR_STRING) {
        return ToQuaternion(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const Color ConfigManager::GetColor(const ea::string& section, const ea::string& parameter, const Color& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_COLOR) {
        return value.GetColor();
    }

    if (value.GetType() == VAR_STRING) {
        return ToColor(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const IntRect ConfigManager::GetIntRect(const ea::string& section, const ea::string& parameter, const IntRect& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_INTRECT) {
        return value.GetIntRect();
    }

    if (value.GetType() == VAR_STRING) {
        return ToIntRect(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const IntVector2 ConfigManager::GetIntVector2(const ea::string& section, const ea::string& parameter, const IntVector2& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_INTVECTOR2) {
        return value.GetIntVector2();
    }

    if (value.GetType() == VAR_STRING) {
        return ToIntVector2(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const Matrix3 ConfigManager::GetMatrix3(const ea::string& section, const ea::string& parameter, const Matrix3& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_MATRIX3) {
        return value.GetMatrix3();
    }

    if (value.GetType() == VAR_STRING) {
        return ToMatrix3(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const Matrix3x4 ConfigManager::GetMatrix3x4(const ea::string& section, const ea::string& parameter, const Matrix3x4& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_MATRIX3X4) {
        return value.GetMatrix3x4();
    }

    if (value.GetType() == VAR_STRING) {
        return ToMatrix3x4(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

const Matrix4 ConfigManager::GetMatrix4(const ea::string& section, const ea::string& parameter, const Matrix4& defaultValue) {
    const Variant value(Get(section, parameter));

    if (value.GetType() == VAR_MATRIX4) {
        return value.GetMatrix4();
    }

    if (value.GetType() == VAR_STRING) {
        return ToMatrix4(value.GetString());
    }

    // Parameter doesn't exist, or is a different type.
    if (saveDefaultParameters_) {
        // Set back to default.
        Set(section, parameter, defaultValue);
    }

    return defaultValue;
}

// Clears all settings.
void ConfigManager::Clear() {
    map_.clear();
}

// Load settings from file.
bool ConfigManager::Load(const ea::string& fileName, bool overwriteExisting) {
    const FileSystem* fileSystem(context_->GetSubsystem<FileSystem>());

    ConfigFile configFile(context_);

    // Check if file exists.
    if (!fileSystem->FileExists(fileName)) {
        return false;
    }

    File file(context_, fileName, FILE_READ);
    configFile.BeginLoad(file);

    return Load(configFile, overwriteExisting);
}

bool ConfigManager::Load(ConfigFile& configFile, bool overwriteExisting) {
    const ConfigMap* map(configFile.GetMap());

    SettingsMap* section(nullptr);
    for (ea::vector<ConfigSection>::const_iterator itr(map->Begin()); itr != map->End(); ++itr) {
        if (itr->Begin() == itr->End()) {
            continue;
        }

        ea::string header("");

        if (itr != map->Begin()) {
            header = *(itr->Begin());
            header = ConfigFile::ParseHeader(header);
        }

        const SettingsMap* section(GetSection(header, true));

        for (ea::vector<ea::string>::const_iterator section_itr = ++itr->Begin(); section_itr != itr->End(); ++section_itr) {
            const ea::string& line(*section_itr);

            ea::string parameter;
            ea::string value;

            ConfigFile::ParseProperty(line, parameter, value);

            if (parameter != "" && value != "") {
                Set(header, parameter, value);
            }
        }
    }

    return true;
}

// Save settings to file.
bool ConfigManager::Save(const ea::string& fileName, bool smartSave) {
    const FileSystem* fileSystem(GetSubsystem<FileSystem>());

    SharedPtr<ConfigFile> configFile(new ConfigFile(context_));

    if (smartSave) {
        SharedPtr<File> file(new File(context_, fileName, FILE_READ));

        // Ensure file is open.
        if (file->IsOpen()) {
            configFile->BeginLoad(*file);
        }
    }

    // Attempt to save the file.
    if (Save(*configFile)) {
        SharedPtr<File> file(new File(context_, fileName, FILE_WRITE));

        // Ensure file is open.
        if (!file->IsOpen()) {
            return false;
        }

        configFile->Save(*file, smartSave);
    }

    return true;
}


bool ConfigManager::Save(ConfigFile& configFile) {
    SaveSettingsMap("", map_, configFile);

    return true;
}

void ConfigManager::SaveSettingsMap(ea::string section, SettingsMap& map, ConfigFile& configFile) {
    // Save out parameters.
    for (SettingsMap::Iterator itr(map.begin()); itr != map.end(); ++itr) {
        // Skip over sub-sections.
        if (itr->second.GetType() == VAR_VOIDPTR) {
            continue;
        }

        ea::string value(itr->first);

        // Set parameter.
        if (itr->second.GetType() == VAR_STRING) {
            configFile.Set(section, value, itr->second.GetString());
        }

        if (itr->second.GetType() == VAR_INT) {
            configFile.Set(section, value, ea::string(itr->second.GetInt()));
        }

        if (itr->second.GetType() == VAR_BOOL) {
            configFile.Set(section, value, ea::string(itr->second.GetBool()));
        }

        if (itr->second.GetType() == VAR_FLOAT) {
            configFile.Set(section, value, ea::string(itr->second.GetFloat()));
        }

        if (itr->second.GetType() == VAR_VECTOR2) {
            configFile.Set(section, value, ea::string(itr->second.GetVector2()));
        }

        if (itr->second.GetType() == VAR_VECTOR3) {
            configFile.Set(section, value, ea::string(itr->second.GetVector3()));
        }

        if (itr->second.GetType() == VAR_VECTOR4) {
            configFile.Set(section, value, ea::string(itr->second.GetVector4()));
        }

        if (itr->second.GetType() == VAR_QUATERNION) {
            configFile.Set(section, value, ea::string(itr->second.GetQuaternion()));
        }

        if (itr->second.GetType() == VAR_COLOR) {
            configFile.Set(section, value, ea::string(itr->second.GetColor()));
        }

        if (itr->second.GetType() == VAR_INTRECT) {
            configFile.Set(section, value, ea::string(itr->second.GetIntRect()));
        }

        if (itr->second.GetType() == VAR_INTVECTOR2) {
            configFile.Set(section, value, ea::string(itr->second.GetIntVector2()));
        }

        if (itr->second.GetType() == VAR_MATRIX3) {
            configFile.Set(section, value, ea::string(itr->second.GetMatrix3()));
        }

        if (itr->second.GetType() == VAR_MATRIX3X4) {
            configFile.Set(section, value, ea::string(itr->second.GetMatrix3x4()));
        }

        if (itr->second.GetType() == VAR_MATRIX4) {
            configFile.Set(section, value, ea::string(itr->second.GetMatrix4()));
        }
    }

    // Save out sub-sections.
    for (SettingsMap::const_iterator itr(map.begin()); itr != map.end(); ++itr) {
        // Skip over parameter.
        if (itr->second.GetType() != VAR_VOIDPTR) {
            continue;
        }

        ea::string path(section);
        path.Append(itr->first);

        SettingsMap* value = static_cast<SettingsMap*>(itr->second.GetVoidPtr());

        if (value) {
            // Save sub-section
            SaveSettingsMap(path, *value, configFile);
        }
    }
}

SettingsMap* ConfigManager::GetSection(const ea::string& section, bool create) {
    // Empty section gets assigned to main map.
    if (section == "") {
        return &map_;
    }

    // Split section into submaps.
    ea::vector<ea::string> split;

    unsigned splitPos(0);

    if (ConfigFile::ParseHeader(section).empty()) {
        return &map_;
    }

    // Split sections by '.' or '/'.
    // Comments will ignore splits behind them.
    while (splitPos != ea::string::NPOS) {
        // Find next comment split
        unsigned commentSplitPos(splitPos);
        unsigned hashPos(section.find("#", commentSplitPos));
        unsigned slashesPos(section.find("//", commentSplitPos));
        commentSplitPos = (hashPos < slashesPos) ? hashPos : slashesPos;

        // Find next split.
        unsigned lastSplitPos(splitPos);
        unsigned dotPos(section.find(".", lastSplitPos));
        unsigned slashPos(section.find("/", lastSplitPos));
        splitPos = (dotPos < slashPos) ? dotPos : slashPos;

        // Ignore splits after comments.
        splitPos = (commentSplitPos <= splitPos) ? ea::string::NPOS : splitPos;

        int length(splitPos - lastSplitPos);
        if (splitPos == ea::string::NPOS) {
            length = section.Length() - lastSplitPos;
        }

        ea::string sub = section.Substring(lastSplitPos, length);

        if (sub != "") {
            split.push_back(sub);
        }
    }

    SettingsMap* currentMap(&map_);
    for (ea::vector<ea::string>::const_iterator itr(split.begin()); itr != split.end(); ++itr) {
        ea::string section(*itr);

        // Find section.
        SettingsMap* newMap(nullptr);
        for (SettingsMap::const_iterator map_itr(currentMap->Begin()); map_itr != currentMap->End(); ++map_itr) {
            if (map_itr->first == section) {
                newMap = static_cast<SettingsMap*>(map_itr->second.GetVoidPtr());

                // Key exists, but is not a SettingsMap.
                if (!newMap) {
                    return nullptr;
                }

                // Key exists.
                break;
            }
        }

        // Key does not exist.
        if (!newMap) {
            if (create) {
                currentMap->operator[](section) = new SettingsMap(); //@TODO: fix a few bytes leak
                newMap = static_cast<SettingsMap*>((*currentMap)[section].GetVoidPtr());
            }
        }

        if (newMap) {
            currentMap = newMap;
        }
    }

    return currentMap;
}

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

#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Resource/Resource.h>

typedef ea::vector<ea::string> ConfigSection;
typedef ea::vector<ConfigSection> ConfigMap;

class ConfigFile : public Urho3D::Resource
{
public:
    URHO3D_OBJECT(ConfigFile, Urho3D::Object);

public:
    ConfigFile(Urho3D::Context* context, bool caseSensitive = false);
    ~ConfigFile() override = default;

    static void RegisterObject(Urho3D::Context* context);

    void SetCaseSensitive(bool caseSensitive) { caseSensitive_ = caseSensitive; }

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Urho3D::Deserializer& source) override;
    /// Save resource.
    virtual bool Save(Urho3D::Serializer& dest) const override;
    /// Smart Save resource, replacing only the values, keeping whitespacing and comments.
    virtual bool Save(Urho3D::Serializer& dest, bool smartSave) const;

    /// Deserialize from a string. Return true if successful.
    bool FromString(const ea::string& source);

    const ConfigMap* GetMap() { return &configMap_; }

    bool Has(const ea::string& section, const ea::string& parameter);

    const ea::string GetString(
        const ea::string& section, const ea::string& parameter, const ea::string& defaultValue = "");

    const int GetInt(const ea::string& section, const ea::string& parameter, const int defaultValue = 0);

    const bool GetBool(const ea::string& section, const ea::string& parameter, const bool defaultValue = false);

    const float GetFloat(const ea::string& section, const ea::string& parameter, const float defaultValue = 0.f);

    const Urho3D::Vector2 GetVector2(const ea::string& section, const ea::string& parameter,
        const Urho3D::Vector2& defaultValue = Urho3D::Vector2::ZERO);

    const Urho3D::Vector3 GetVector3(const ea::string& section, const ea::string& parameter,
        const Urho3D::Vector3& defaultValue = Urho3D::Vector3::ZERO);

    const Urho3D::Vector4 GetVector4(const ea::string& section, const ea::string& parameter,
        const Urho3D::Vector4& defaultValue = Urho3D::Vector4::ZERO);

    const Urho3D::Quaternion GetQuaternion(const ea::string& section, const ea::string& parameter,
        const Urho3D::Quaternion& defaultValue = Urho3D::Quaternion::IDENTITY);

    const Urho3D::Color GetColor(const ea::string& section, const ea::string& parameter,
        const Urho3D::Color& defaultValue = Urho3D::Color::WHITE);

    const Urho3D::IntRect GetIntRect(const ea::string& section, const ea::string& parameter,
        const Urho3D::IntRect& defaultValue = Urho3D::IntRect::ZERO);

    const Urho3D::IntVector2 GetIntVector2(const ea::string& section, const ea::string& parameter,
        const Urho3D::IntVector2& defaultValue = Urho3D::IntVector2::ZERO);

    const Urho3D::Matrix3 GetMatrix3(const ea::string& section, const ea::string& parameter,
        const Urho3D::Matrix3& defaultValue = Urho3D::Matrix3::IDENTITY);

    const Urho3D::Matrix3x4 GetMatrix3x4(const ea::string& section, const ea::string& parameter,
        const Urho3D::Matrix3x4& defaultValue = Urho3D::Matrix3x4::IDENTITY);

    const Urho3D::Matrix4 GetMatrix4(const ea::string& section, const ea::string& parameter,
        const Urho3D::Matrix4& defaultValue = Urho3D::Matrix4::IDENTITY);

    void Set(const ea::string& section, const ea::string& parameter, const ea::string& value);

    // Returns header without bracket.
    static const ea::string ParseHeader(ea::string line);
    // Set property; Empty if no property.
    static const void ParseProperty(ea::string line, ea::string& property, ea::string& value);
    // Strip comments and whitespace.
    static const ea::string ParseComments(ea::string line);

protected:
    bool caseSensitive_;
    ConfigMap configMap_;
};

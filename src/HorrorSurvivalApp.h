//
// Copyright (c) 2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <Urho3D/Engine/PluginApplication.h>

class HorrorSurvivalApp : public Urho3D::PluginApplication
{
public:
	HorrorSurvivalApp(Urho3D::Context* context);

    /// Called when plugin is being loaded. Register all custom components and subscribe to events here.
    virtual void Load() override;
    /// Called when application is started. May be called multiple times but no earlier than before next Stop() call.
    virtual void Start() override;
    /// Called when application is stopped.
    virtual void Stop() override;
    /// Called when plugin is being unloaded. Unregister all custom components and unsubscribe from events here.
    virtual void Unload() override;

    void HandleLogMessage(Urho3D::StringHash eventType, Urho3D::VariantMap& args);
};

#ifdef PACKAGE_MANAGER
    #include <Urho3D/Core/Context.h>
    #include <Urho3D/IO/FileSystem.h>
    #include <Urho3D/IO/Log.h>
    #include <Urho3D/IO/PackageFile.h>
    #include <Urho3D/Resource/ResourceCache.h>

    #if !defined(__EMSCRIPTEN__)
        #include <Urho3D/Network/Network.h>
    #endif

    #include "../Config/ConfigManager.h"
    #include "../Console/ConsoleHandlerEvents.h"
    #include "PackageManager.h"
    #include "PackageManagerEvents.h"
    #include <Urho3D/Core/CoreEvents.h>

using namespace Urho3D;
using namespace ConsoleHandlerEvents;

PackageManager::PackageManager(Context* context)
    : Object(context)
{
}

PackageManager::~PackageManager() {}

void PackageManager::RegisterObject(Context* context) { context->RegisterFactory<PackageManager>(); }

void PackageManager::Init() { SubscribeToEvents(); }

void PackageManager::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PackageManager, HandleUpdate));

    SendEvent(E_CONSOLE_COMMAND_ADD, ConsoleCommandAdd::P_NAME, "package_manager_list", ConsoleCommandAdd::P_EVENT,
        "#package_manager_list", ConsoleCommandAdd::P_DESCRIPTION, "ea::list all available packages",
        ConsoleCommandAdd::P_OVERWRITE, true);
    SubscribeToEvent("#package_manager_list",
        [&](StringHash eventType, VariantMap& eventData)
        {
            StringVector params = eventData["Parameters"].GetStringVector();
            if (params.size() != 2)
            {
                URHO3D_LOGERROR("You must provide url");
                return;
            }
            if (httpRequest_.Null())
            {
                URHO3D_LOGINFOF("Listing all available packages from %s", params[1].c_str());
    #if !defined(__EMSCRIPTEN__)
                auto* network = GetSubsystem<Network>();
                data_.clear();
                httpRequest_ = network->MakeHttpRequest(params[1]);
    #endif
            }
        });
}

void PackageManager::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    #if !defined(__EMSCRIPTEN__)
    if (!httpRequest_.Null())
    {
        // Initializing HTTP request
        if (httpRequest_->GetState() == HTTP_INITIALIZING)
        {
            return;
            // An error has occurred
        }
        else if (httpRequest_->GetState() == HTTP_ERROR)
        {
            URHO3D_LOGERRORF("HttpRequest error: %s", httpRequest_->GetError().c_str());
            httpRequest_.Reset();
        }
        else
        {
            if (httpRequest_->GetAvailableSize() > 0)
            {
                data_ += httpRequest_->ReadLine();
                URHO3D_LOGINFOF("Reading data: %s", data_.c_str());
            }
            else
            {
                SharedPtr<JSONFile> json(new JSONFile(context_));
                json->FromString(data_);
                URHO3D_LOGINFOF("Request returned: %s", json->ToString().c_str());
                httpRequest_.Reset();
            }
        }
    }
    #endif
}

#endif
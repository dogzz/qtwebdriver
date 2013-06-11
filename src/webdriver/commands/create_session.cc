// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "commands/create_session.h"

#include <string>

#include "base/file_path.h"
#include "base/scoped_temp_dir.h"
#include "base/values.h"
#include "base/bind.h"
#include "commands/response.h"
#include "webdriver_error.h"
#include "webdriver_session.h"
#include "webdriver_session_manager.h"
#include "webdriver_view_executor.h"
#include "webdriver_view_enumerator.h"
#include "webdriver_view_factory.h"

namespace webdriver {

CreateSession::CreateSession(const std::vector<std::string>& path_segments,
                             const DictionaryValue* const parameters)
    : Command(path_segments, parameters) {}

CreateSession::~CreateSession() {}

bool CreateSession::DoesPost() { return true; }

void CreateSession::ExecutePost(Response* const response) {
    const DictionaryValue* dict;
    if (!GetDictionaryParameter("desiredCapabilities", &dict)) {
        response->SetError(new Error(
            kBadRequest, "Missing or invalid 'desiredCapabilities'"));
        return;
    }

    if (SessionManager::GetInstance()->GetSessions().size()  > 0) {
        response->SetError(new Error(kUnknownError, "Cannot start session. WD support only one session at the moment"));
        return;
    }

    // Session manages its own liftime, so do not call delete.
    Session* session = new Session();
    Error* error = session->Init(dict);
    if (error) {
        response->SetError(error);
        return;
    }

    std::string browser_start_window = session->capabilities().browser_start_window;
    std::string window_class = session->capabilities().browser_class;
    ViewId startView;

    session->logger().Log(kInfoLogLevel,
        "Session("+session->id()+
        ") browser class("+window_class+
        ") start window("+browser_start_window+").");

    if (!browser_start_window.empty()) {
        // enumerate all views
        session->logger().Log(kFineLogLevel, "Trying to attach to window - "+browser_start_window);

        std::vector<ViewId> views;

        session->RunSessionTask(base::Bind(
            &ViewEnumerator::EnumerateViews,
            session,
            &views));

        // TODO: implement attach functionality
        // find view to attach
    }

    if (!startView.is_valid()) {
        session->logger().Log(kFineLogLevel, "Trying to create window - "+window_class);

        ViewHandle* viewHandle = NULL;
        // create view
        session->RunSessionTask(base::Bind(
            &ViewFactory::CreateViewByClassName,
            base::Unretained(ViewFactory::GetInstance()),
            session->logger(),
            window_class,
            &viewHandle));

        if (NULL != viewHandle) {
            session->AddNewView(viewHandle, &startView);
            if (!startView.is_valid()) {
                viewHandle->Release();
                session->logger().Log(kSevereLogLevel, "Cant add view handle to session.");
            }
        }
    }

    if (!startView.is_valid()) {
        session->logger().Log(kSevereLogLevel, "Session("+session->id()+") no view ids.");
        response->SetError(new Error(kUnknownError, "No view ids after initialization"));
        session->Terminate();
        return;   
    }

    // set current view
    error = SwitchToView(session, startView);
    if (error) {
        response->SetError(error);
        session->Terminate();
        return;
    }

    // Redirect to a relative URI. Although prohibited by the HTTP standard,
    // this is what the IEDriver does. Finding the actual IP address is
    // difficult, and returning the hostname causes perf problems with the python
    // bindings on Windows.
    std::ostringstream stream;
    stream << SessionManager::GetInstance()->url_base() << "/session/"
            << session->id();
    response->SetStatus(kSeeOther);
    response->SetValue(Value::CreateStringValue(stream.str()));
}

Error* CreateSession::SwitchToView(Session* session, const ViewId& viewId) {
    typedef scoped_ptr<ViewCmdExecutor> ExecutorPtr;        

    Error* error = NULL;
    ExecutorPtr executor(ViewCmdExecutorFactory::GetInstance()->CreateExecutor(session, viewId));

    if (NULL == executor.get()) {
        return new Error(kBadRequest, "cant get view executor.");
    }

    session->logger().Log(kFineLogLevel, "start view ("+viewId.id()+")");

    session->RunSessionTask(base::Bind(
                &ViewCmdExecutor::SwitchTo,
                base::Unretained(executor.get()),
                &error));

    return error;
}

}  // namespace webdriver

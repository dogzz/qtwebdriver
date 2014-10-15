/****************************************************************************
**
** Copyright © 1992-2014 Cisco and/or its affiliates. All rights reserved.
** All rights reserved.
** 
** $CISCO_BEGIN_LICENSE:LGPL$
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** $CISCO_END_LICENSE$
**
****************************************************************************/

#include "webdriver_view_runner.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/synchronization/waitable_event.h"

namespace webdriver {

ViewRunner::CreateRunnerMethod ViewRunner::create = &(ViewRunner::createRunner<ViewRunner>);

ViewRunner::ViewRunner() {}

void ViewRunner::RunClosure(const base::Closure& task, base::WaitableEvent* done_event) {
	task.Run();
    done_event->Signal();
}

ViewRunner* ViewRunner::CreateRunner(void) {
	return create();
}

} //namespace webdriver 

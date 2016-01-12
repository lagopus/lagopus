/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LAGOPUS_DATASTORE_H__
#define __LAGOPUS_DATASTORE_H__

/**
 * @file	datastore.h
 */
#include "datastore/common.h"
#include "datastore/bridge.h"
#include "datastore/channel.h"
#include "datastore/port.h"
#include "datastore/interface.h"
#include "datastore/controller.h"
#include "datastore/affinition.h"
#include "datastore/queue.h"
#include "datastore/policer.h"
#include "datastore/policer_action.h"
#include "datastore/datastore_config.h"
#include "datastore/datastore_module.h"
#ifdef HYBRID
#include "datastore/mactable_cmd.h"
#endif /* HYBRID */
#endif /* ! __LAGOPUS_DATASTORE_H__ */

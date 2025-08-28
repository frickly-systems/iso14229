#pragma once

#ifdef __ZEPHYR__

#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#ifndef static_assert
#define static_assert(cond, msg) BUILD_ASSERT((cond), msg)
#endif // static_assert

#else

#include <assert.h>
#include <sys/types.h>

#if !defined(static_assert) && defined(__GNUC__)
#define static_assert(cond, msg) _Static_assert((cond), msg)
#endif // !defined(static_assert) && defined(__GNUC__)

#endif // __ZEPHYR__
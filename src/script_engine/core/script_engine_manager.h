/**
 * @file script_engine_manager.h
 * @brief Script engine manager - high-level semantic operations
 *
 * Provides encapsulated operations for watchface lifecycle (pause/resume/restart)
 * and app lifecycle (run/stop). External subsystems MUST include this header
 * and use only these APIs. The underlying core APIs are re-exported for type
 * access but context handles are managed internally.
 */
#ifndef SCRIPT_ENGINE_MANAGER_H
#define SCRIPT_ENGINE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ---------------------------------------------------*/
#include "script_engine_core.h"

/* Public function prototypes --------------------------------*/

/**
 * @brief Start a watchface script
 * @param pkg Script package to run
 * @return script_engine_result_t
 *
 * Creates a context handle internally and runs the script.
 * If an existing watchface context is active, it is cleaned up first.
 */
script_engine_result_t script_engine_watchface_start(const script_pkg_t *pkg);

/**
 * @brief Pause the current watchface script (save context)
 * @return script_engine_result_t
 *
 * Saves the current runtime context (realm + state) for later restoration.
 * The engine returns to idle after this call.
 */
script_engine_result_t script_engine_watchface_pause(void);

/**
 * @brief Resume the current watchface script (restore context)
 * @return script_engine_result_t
 *
 * Restores the previously saved runtime context.
 * The engine resumes from IDLE state with all callbacks intact.
 */
script_engine_result_t script_engine_watchface_resume(void);

/**
 * @brief Destroy the current watchface context and stop
 * @return script_engine_result_t
 *
 * Stops the engine and destroys the watchface context handle.
 * Called when switching to a different watchface or shutting down.
 */
script_engine_result_t script_engine_watchface_destroy(void);

/**
 * @brief Check if watchface has saved context
 * @return true if a saved watchface context exists
 */
bool script_engine_watchface_has_context(void);

/**
 * @brief Run an application script
 * @param pkg Script package to run
 * @return script_engine_result_t
 *
 * If the engine is in SUSPENDED state (from a paused watchface),
 * the saved context is discarded before running the app.
 */
script_engine_result_t script_engine_app_run(const script_pkg_t *pkg);

/**
 * @brief Stop the current application script
 * @return script_engine_result_t
 */
script_engine_result_t script_engine_app_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* SCRIPT_ENGINE_MANAGER_H */

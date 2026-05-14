/**
 * @file eos_chrome_manager.h
 * @brief System chrome manager - centralized overlay and crown button management
 *
 * Manages system-level overlays (msg_list, control_center, etc.) that sit on
 * top of regular app content. Provides a single entry point for crown button
 * interactions, eliminating direct coupling between crown input and overlay widgets.
 */

#ifndef EOS_CHROME_MANAGER_H
#define EOS_CHROME_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ---------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Public typedefs --------------------------------------------*/

/**
 * @brief Overlay descriptor for registration with the chrome manager
 */
typedef struct {
    bool (*is_open)(void);      /**< Check if this overlay is currently open */
    void (*pull_back)(void);    /**< Pull back (close with animation) this overlay */
    void (*hide)(void);         /**< Hide this overlay immediately (no animation) */
    void (*on_hidden)(void);    /**< Called when overlay is hidden (for stack sync) */
    const char *name;           /**< Debug name for logging */
} eos_chrome_overlay_t;

/* Public function prototypes --------------------------------*/

/**
 * @brief Initialize the chrome manager
 * @note Must be called after msg_list and control_center are initialized
 */
void eos_chrome_manager_init(void);

/**
 * @brief Register a system overlay for unified management
 * @param overlay Overlay descriptor (must remain valid for lifetime)
 */
void eos_chrome_manager_register_overlay(const eos_chrome_overlay_t *overlay);

/**
 * @brief Check if any registered overlay is currently open
 * @return true if at least one overlay is open
 */
bool eos_chrome_manager_any_overlay_open(void);

/**
 * @brief Get top overlay without removing it
 * @return Pointer to top overlay, NULL if stack is empty
 */
const eos_chrome_overlay_t *eos_chrome_manager_get_top_overlay(void);

/**
 * @brief Pull back (close with animation) the topmost overlay
 */
void eos_chrome_manager_pull_back_top(void);

/**
 * @brief Pull back (close with animation) all open overlays
 */
void eos_chrome_manager_pull_back_all(void);

/**
 * @brief Notify chrome manager that an overlay has been opened
 * @param name Name of the overlay (must match registered name)
 */
void eos_chrome_manager_notify_overlay_opened(const char *name);

/**
 * @brief Push an overlay to the stack (called when overlay opens)
 * @param overlay Overlay to push
 */
void eos_chrome_manager_push_overlay(const eos_chrome_overlay_t *overlay);

/**
 * @brief Remove an overlay from the stack (called when overlay closes)
 * @param overlay Overlay to remove
 */
void eos_chrome_manager_remove_overlay(const eos_chrome_overlay_t *overlay);

/**
 * @brief Handle a crown button click event
 *
 * Decision logic:
 *   1. If PM is sleeping → wake up, return
 *   2. If any overlay is open → pull back topmost, return (no navigation)
 *   3. If on watchface → enter app list
 *   4. Otherwise → navigate back
 */
void eos_chrome_manager_handle_crown_click(void);

/**
 * @brief Handle activity switch (called during activity transitions)
 *
 * Pulls back any open overlays (with animation) when leaving the watchface,
 * then hides all overlays for the new activity.
 */
void eos_chrome_manager_handle_activity_switch(void);

#ifdef __cplusplus
}
#endif

#endif /* EOS_CHROME_MANAGER_H */

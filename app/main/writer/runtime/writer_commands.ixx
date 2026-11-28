/**
 * \file writer_commands.ixx
 * \brief Runtime command registry for `auto_core_writer.exe`.
 */
export module writer_commands;

import command_registry;
import std;
import auto_core.core.console;

namespace writer_actions {
    void select_and_insert_session_prompt();
    void select_and_insert_text();
    void select_and_insert_profile_url();
    void create_or_open_daily_note_in_notepad();
    void launch_task_list();
    void print_timestamp();
    void print_date_iso();
    void print_date_compact();
    void print_date_iso_with_timestamp();
    void print_date_iso_with_timestamp_w();
    void add_brackets_around_clipboard();

    /**
     * \brief Exclusive lease for a Writer menu that reads standard input.
     *
     * `try_acquire` fails while another lease is held. The destructor
     * releases the lease. Move the lease into the menu thread so thread
     * creation failure and thread completion both release it once.
     */
    class SelectionLease {
    public:
        class Token {
            friend class SelectionLease;
            Token() = default;
        };

        SelectionLease(const SelectionLease&) = delete;
        SelectionLease& operator=(const SelectionLease&) = delete;

        SelectionLease(SelectionLease&& other) noexcept
            : held_ {std::exchange(other.held_, false)} {}

        SelectionLease& operator=(SelectionLease&&) = delete;

        ~SelectionLease() { release(); }

        explicit SelectionLease(Token) {}

        [[nodiscard]] static std::optional<SelectionLease> try_acquire() {
            bool expected = false;
            if (!active().compare_exchange_strong(
                    expected,
                    true,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                return std::nullopt;
            }
            return std::optional<SelectionLease>(std::in_place, Token {});
        }

    private:
        void release() noexcept {
            if (!held_) {
                return;
            }
            held_ = false;
            active().store(false, std::memory_order_release);
        }

        static std::atomic_bool& active() {
            static std::atomic_bool flag {false};
            return flag;
        }

        bool held_ = true;
    };

    /**
     * \brief Restores the window that was active before a Writer prompt.
     *
     * Construct after `ac::console::focus_for_prompt_via_winkey()`
     * succeeds. The destructor restores that window while the guard is
     * armed. Call `release()` after
     * `insert_text_preserving_clipboard_text()` returns normally so a
     * successful insert is not activated again.
     *
     * That insertion function returns `void`, so a normal return after
     * activation failure cannot be distinguished from success. The guard
     * does not restore in that case. The destructor does not throw.
     */
    class PromptFocus {
    public:
        explicit PromptFocus(const ac::console::WindowHandle window) noexcept
            : window_ {window} {}

        PromptFocus(const PromptFocus&) = delete;
        PromptFocus& operator=(const PromptFocus&) = delete;
        PromptFocus(PromptFocus&&) = delete;
        PromptFocus& operator=(PromptFocus&&) = delete;

        ~PromptFocus() noexcept;

        void release() noexcept { armed_ = false; }

    private:
        ac::console::WindowHandle window_;
        bool armed_ = true;
    };

    [[nodiscard]] std::optional<std::size_t> select_numbered_entry(
        const std::string& menu,
        std::size_t count,
        const char* kind
    );
}

export command_registry::Registry create_writer_command_registry();

/**
 * \file writer_commands.ixx
 * \brief Runtime command registry for `writer_ac.exe`.
 */
export module writer_commands;

import command_registry;
import std;

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
}

export command_registry::Registry create_writer_command_registry();

#pragma once
// Moonraker client for mcu-cnc-pendant on its own FreeRTOS task: esp_http_client blocks
// (DNS is not bounded by timeout_ms), so the main loop only queues commands and reads
// the last polled status. An unreachable Klipper shows as "offline" instead of starving
// the loop into the task watchdog.

#include <deque>
#include <mutex>
#include <string>

#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esphome/components/json/json_util.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace moonraker {

struct Status {
  float pos[3]{};
  std::string homed;
  std::string state{"offline"};
};

class Client {
 public:
  void start(const char *base) {
    this->base_ = base;
    xTaskCreate(task, "moonraker", 6144, this, 1, nullptr);
  }

  // Run G-code; dropped when Klipper is not keeping up.
  void gcode(const std::string &script) {
    JsonDocument doc;
    doc["script"] = script;
    std::string body;
    serializeJson(doc, body);
    this->post("/printer/gcode/script", body);
  }

  // urgent: throw away everything queued and send this first (emergency stop)
  void post(const std::string &path, const std::string &body = "", bool urgent = false) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    if (urgent) {
      this->queue_.clear();
    } else if (this->queue_.size() >= MAX_QUEUE) {
      return;
    }
    this->queue_.emplace_back(path, body);
  }

  Status status() {
    std::lock_guard<std::mutex> lock(this->mutex_);
    return this->status_;
  }

 protected:
  static constexpr size_t MAX_QUEUE = 8;
  static constexpr uint32_t POLL_MS = 250;
  static constexpr int TIMEOUT_MS = 1000;

  static void task(void *arg) {
    auto *self = static_cast<Client *>(arg);
    uint32_t last_poll = 0;
    for (;;) {
      std::pair<std::string, std::string> cmd;
      bool have_cmd = false;
      {
        std::lock_guard<std::mutex> lock(self->mutex_);
        if (!self->queue_.empty()) {
          cmd = std::move(self->queue_.front());
          self->queue_.pop_front();
          have_cmd = true;
        }
      }
      if (have_cmd) {
        int code = self->request(cmd.first, &cmd.second, nullptr);
        if (code != 200)
          ESP_LOGW("moonraker", "POST %s failed (%d): %s", cmd.first.c_str(), code, cmd.second.c_str());
        continue;  // drain commands before the next poll
      }
      if (esphome::millis() - last_poll >= POLL_MS) {
        last_poll = esphome::millis();
        self->poll();
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }

  void poll() {
    std::string body;
    Status s;
    if (this->request("/printer/objects/query?gcode_move=gcode_position&toolhead=homed_axes&print_stats=state",
                      nullptr, &body) == 200) {
      auto doc = esphome::json::parse_json(body);
      JsonObject st = doc["result"]["status"];
      JsonArray p = st["gcode_move"]["gcode_position"];
      for (int i = 0; i < 3; i++)
        s.pos[i] = p[i];
      s.homed = st["toolhead"]["homed_axes"].as<std::string>();
      s.state = st["print_stats"]["state"].as<std::string>();
    } else {
      std::lock_guard<std::mutex> lock(this->mutex_);
      s.pos[0] = this->status_.pos[0];  // keep the last position, only flag offline
      s.pos[1] = this->status_.pos[1];
      s.pos[2] = this->status_.pos[2];
    }
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->status_ = std::move(s);
  }

  // POST when body is set, GET otherwise; returns the HTTP status or -1
  int request(const std::string &path, const std::string *body, std::string *out) {
    std::string url = this->base_ + path;
    esp_http_client_config_t cfg{};
    cfg.url = url.c_str();
    cfg.timeout_ms = TIMEOUT_MS;
    cfg.method = body ? HTTP_METHOD_POST : HTTP_METHOD_GET;
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (c == nullptr)
      return -1;
    int code = -1;
    int len = body ? body->size() : 0;
    if (len > 0)
      esp_http_client_set_header(c, "Content-Type", "application/json");
    if (esp_http_client_open(c, len) == ESP_OK && (len == 0 || esp_http_client_write(c, body->data(), len) == len)) {
      esp_http_client_fetch_headers(c);
      code = esp_http_client_get_status_code(c);
      char buf[256];
      int n;
      while (out != nullptr && (n = esp_http_client_read(c, buf, sizeof(buf))) > 0)
        out->append(buf, n);
    }
    esp_http_client_cleanup(c);
    return code;
  }

  std::string base_;
  std::mutex mutex_;
  std::deque<std::pair<std::string, std::string>> queue_;
  Status status_;
};

inline Client client;

}  // namespace moonraker

#pragma once
#include "services/AdapterService.h"
namespace iot {
class AdapterController {
public:
    explicit AdapterController(std::shared_ptr<AdapterService> service) : service_(std::move(service)) {}
    void registerRoutes() const;
private:
    std::shared_ptr<AdapterService> service_;
};
}

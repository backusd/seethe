#pragma once

namespace seethe
{
class Application;

class ChangeRequest
{
public:
	ChangeRequest() noexcept = default;
	ChangeRequest(const ChangeRequest&) noexcept = default;
	ChangeRequest(ChangeRequest&&) noexcept = default;
	ChangeRequest& operator=(const ChangeRequest&) noexcept = default;
	ChangeRequest& operator=(ChangeRequest&&) noexcept = default;
	virtual ~ChangeRequest() noexcept = default;

	virtual void Undo(Application*) noexcept = 0;
	virtual void Redo(Application*) noexcept = 0;
};
}
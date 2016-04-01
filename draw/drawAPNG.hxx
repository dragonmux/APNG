#ifndef DRAW_APNG__HXX
#define DRAW_APNG__HXX

#include <stdint.h>
#include <thread>
#include <atomic>
#include <apng.hxx>
#include "ui_drawAPNG.h"

class drawAPNG_t final : public QMainWindow
{
	Q_OBJECT

private:
	Ui::drawAPNG_t window;
	std::atomic<bool> leave;
	std::thread animateThread;
	std::unique_ptr<apng_t> apng;
	std::vector<QPixmap> frames;
	std::vector<displayTime_t> displayTimings;

	QImage::Format pixelFormat(const pixelFormat_t format) const noexcept;
	void processFrame(const bitmap_t *const frame, QImage &dest) noexcept;
	void animate() noexcept;

public:
	explicit drawAPNG_t(QWidget *parent = nullptr) noexcept;
	~drawAPNG_t() noexcept;
	void image(std::unique_ptr<apng_t> &&image) noexcept;
};

#endif /*DRAW_APNG__HXX*/

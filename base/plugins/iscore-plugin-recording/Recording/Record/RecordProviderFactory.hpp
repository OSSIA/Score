#pragma once
#include <Recording/Record/RecordTools.hpp>
#include <Device/Node/DeviceNode.hpp>
#include <Explorer/Explorer/DeviceExplorerModel.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Scenario/Palette/ScenarioPoint.hpp>
#include <iscore/plugins/customfactory/FactoryInterface.hpp>
#include <iscore/serialization/VisitorCommon.hpp>
#include <iscore_plugin_recording_export.h>

namespace Scenario
{
class ProcessModel;
}
namespace Explorer
{
class DeviceExplorerModel;
}
namespace Recording
{
using Priority = int;
struct RecordContext
{
        using clock = std::chrono::steady_clock;
        RecordContext(const Scenario::ProcessModel&, Scenario::Point pt);
        RecordContext(const RecordContext& other) = delete;
        RecordContext(RecordContext&& other) = delete;
        RecordContext& operator=(const RecordContext& other) = delete;
        RecordContext& operator=(RecordContext&& other) = delete;

        void start()
        {
            firstValueTime = clock::now();
            timer.start();
        }

        bool started() const
        {
            return firstValueTime.time_since_epoch() != clock::duration::zero();
        }

        TimeValue time() const
        {
            return GetTimeDifference(firstValueTime);
        }

        double timeInDouble() const
        {
            return GetTimeDifferenceInDouble(firstValueTime);
        }

        const iscore::DocumentContext& context;
        const Scenario::ProcessModel& scenario;
        Explorer::DeviceExplorerModel& explorer;
        RecordCommandDispatcher dispatcher;

        Scenario::Point point;
        clock::time_point firstValueTime;
        QTimer timer;
};

struct RecordProvider
{
        virtual ~RecordProvider();
        virtual void setup(const Box&, const RecordListening&) = 0;
        virtual void stop() = 0;
};

// A recording session.
class Recorder
{
    public:
        Recorder(RecordContext& ctx):
            m_context{ctx}
        {
            // Algorithm :
            // 1. Get all selected addresses.
            // 2. Generate the concrete recorders that match these addresses.
            // Use settings to select them recursively or not..
            // 3. Create a box corresponding to the case where we record
            // in the void, in an existing constraint, starting from a state / event...

        }

        void setup()
        {

        }

        void stop()
        {

        }

    private:
        RecordContext& m_context;
        std::vector<std::unique_ptr<RecordProvider>> m_recorders;
};

// Sets-up the recording of a single class.
template<typename T>
class SingleRecorder :
        public QObject
{
    public:
        T recorder;

        SingleRecorder(RecordContext& ctx):
            recorder{ctx}
        {
        }

        void setup()
        {
            auto& ctx = recorder.context;
            //// Device tree management ////
            // Get the listening of the selected addresses
            auto recordListening = GetAddressesToRecordRecursive(ctx.explorer);
            if(recordListening.empty())
                return;

            // Disable listening for everything
            ctx.explorer.deviceModel().listening().stop();

            // Create the processes, etc.
            Box box = CreateBox(ctx);

            recorder.setup(box, recordListening);

            //// Start the record timer ////
            ctx.timer.setTimerType(Qt::PreciseTimer);
            ctx.timer.setInterval(16.66 * 4); // TODO ReasonableUpdateInterval(curve_count));
            QObject::connect(&ctx.timer, &QTimer::timeout,
                    this, [&] () {

                // Move end event by the current duration.
                box.moveCommand.update(
                            {},
                            {},
                            box.endEvent,
                            ctx.point.date + GetTimeDifference(ctx.firstValueTime),
                            0,
                            true);

                box.moveCommand.redo();
            });

            // In case where the software is exited
            // during recording.
            QObject::connect(&ctx.scenario, &IdentifiedObjectAbstract::identified_object_destroyed,
                    this, [&] () {
                ctx.timer.stop();
            });
        }

        void stop()
        {
            auto& ctx = recorder.context;
            ctx.timer.stop();

            recorder.stop();

            ctx.explorer.deviceModel().listening().restore();    // Commit
            ctx.dispatcher.commit();
        }

    private:
};


class ISCORE_PLUGIN_RECORDING_EXPORT RecorderFactory :
        public iscore::AbstractFactory<RecorderFactory>
{
    ISCORE_ABSTRACT_FACTORY("64999184-a705-4686-b967-14e8f79692f1")
    public:
        virtual ~RecorderFactory();
        /**
         * @brief matches
         * @return <= 0 : does not match
         * > 0 : matches. The highest priority should be taken.
         */
        virtual Priority matches(
            const Device::Node&,
            const iscore::DocumentContext& ctx) = 0;

        virtual std::unique_ptr<RecordProvider> make(
            const Device::NodeList&,
            const iscore::DocumentContext& ctx) = 0;
};

}

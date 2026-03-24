
#pragma once

#include "rhi/include/RHICommandBuffer.h"
#include "thread/ThreadPool.h"
#include <vector>
#include <functional>
#include <future>
#include <thread>

namespace engine {
namespace rhi {

// ===================================================================
// 步骤 7: ParallelRecorder - 多线程命令录制工具
//
// 允许多个 RenderPass 在不同线程上并行录制命令到各自独立的 CommandBuffer。
// 录制完成后，由主线程按指定顺序提交到 CommandQueue。
//
// 重要限制：
//   - 仅在纯录制模式下可用（m_ImmediateDevice = nullptr）
//   - 在即时执行模式下（当前过渡阶段），仍走单线程录制
//   - 各 CommandBuffer 互相独立，无共享状态，天然线程安全
//
// 用法：
//   ParallelRecorder recorder;
//   recorder.addJob([&](CommandBuffer& cmd) {
//       cmd.beginRenderPass(target1, params1);
//       cmd.bindPipeline(pipeline1);
//       cmd.draw(100, 0);
//       cmd.endRenderPass();
//   });
//   recorder.addJob([&](CommandBuffer& cmd) {
//       cmd.beginRenderPass(target2, params2);
//       cmd.bindPipeline(pipeline2);
//       cmd.draw(200, 0);
//       cmd.endRenderPass();
//   });
//   recorder.execute();  // 并行执行所有 job
//   // 获取结果
//   for (auto& cmdBuf : recorder.getResults()) {
//       queue.submit(cmdBuf);
//   }
// ===================================================================

class ParallelRecorder {
public:
    using RecordJob = std::function<void(CommandBuffer&)>;

    ParallelRecorder() = default;

    // 添加一个录制任务
    void addJob(RecordJob job) {
        m_Jobs.push_back(std::move(job));
    }

    // 并行执行所有录制任务
    // 每个 job 在独立的 CommandBuffer 上执行
    void execute() {
        size_t jobCount = m_Jobs.size();
        m_Results.resize(jobCount);

        if (jobCount == 0) return;

        // 单任务时直接在当前线程执行
        if (jobCount == 1) {
            m_Results[0].reset();
            m_Jobs[0](m_Results[0]);
            return;
        }

        // 多任务时使用 std::async 并行执行
        // （未来可以切换为使用全局 thread_pool，但 std::async 已经足够好）
        std::vector<std::future<void>> futures;
        futures.reserve(jobCount);

        for (size_t i = 0; i < jobCount; ++i) {
            m_Results[i].reset();
            futures.push_back(std::async(std::launch::async,
                [this, i]() {
                    m_Jobs[i](m_Results[i]);
                }));
        }

        // 等待所有任务完成
        for (auto& f : futures) {
            f.get();
        }
    }

    // 使用全局 ThreadPool 并行执行（替代 std::async）
    void executeWithThreadPool() {
        size_t jobCount = m_Jobs.size();
        m_Results.resize(jobCount);

        if (jobCount == 0) return;

        if (jobCount == 1) {
            m_Results[0].reset();
            m_Jobs[0](m_Results[0]);
            return;
        }

        // 使用全局 thread_pool 分发任务
        for (size_t i = 0; i < jobCount; ++i) {
            m_Results[i].reset();
            thread_pool.addTask(new TextureLoadTask([this, i]() {
                m_Jobs[i](m_Results[i]);
            }));
        }

        // 等待所有任务完成
        thread_pool.wait();
    }

    // 获取录制结果
    const std::vector<CommandBuffer>& getResults() const { return m_Results; }
    std::vector<CommandBuffer>& getResults() { return m_Results; }

    // 获取指定索引的 CommandBuffer
    CommandBuffer& getResult(size_t index) { return m_Results[index]; }
    const CommandBuffer& getResult(size_t index) const { return m_Results[index]; }

    // 清空所有 job 和结果
    void clear() {
        m_Jobs.clear();
        m_Results.clear();
    }

    // 获取 job 数量
    size_t jobCount() const { return m_Jobs.size(); }

private:
    std::vector<RecordJob> m_Jobs;
    std::vector<CommandBuffer> m_Results;
};

} // namespace rhi
} // namespace engine

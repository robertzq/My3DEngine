#pragma once
#include <memory>
#include <vector>

class Menu;

// 轻量菜单栈（Step 7）。
//
// Ownership：MenuStack 独占其持有的 Menu（unique_ptr）。
//   * Push/Replace 接管传入 unique_ptr 的所有权。
//   * Pop/Replace/Clear 会析构被移除的 Menu。
//   * Top()/栈内遍历返回非拥有指针，生命周期由 MenuStack 决定。
//
// 行为：
//   * Push 后下层 Menu 保留（仍在栈中、仍渲染）但 SetActive(false)：不响应输入、不显示 focus。
//   * Pop 后恢复新 top 的 active/focus。
//   * 只有 top 接收输入（MenuInput）。
class MenuStack {
public:
    void Push(std::unique_ptr<Menu> menu);        // 接管并置为 top
    bool Pop();                                   // 销毁 top，激活新 top；空栈返回 false
    void Replace(std::unique_ptr<Menu> menu);     // 销毁当前 top 后压入新 menu
    void Clear();                                 // 销毁全部，安全

    Menu* Top() const;                            // 非拥有
    std::size_t Size() const { return stack.size(); }
    bool Empty() const { return stack.empty(); }

    void HandleInput(float dt);                   // 仅 top 响应
    void Update(float dt);                        // 全部 update（布局）
    void Render();                                // 从底到顶绘制

private:
    void ActivateTop();

    std::vector<std::unique_ptr<Menu>> stack;
};

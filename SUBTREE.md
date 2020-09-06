# Update from remote

git remote add origin-implot https://github.com/epezent/implot.git
git remote add origin-imgui https://github.com/ocornut/imgui.git
git subtree pull --prefix implot origin-implot master --squash
git subtree pull --prefix imgui origin-imgui docking --squash

# Update from remote

git remote add origin-implot git@github.com:nburrus/implot.git
git remote add origin-imgui git@github.com:nburrus/imgui.git
git subtree pull --prefix implot origin-implot cvlog --squash
git subtree pull --prefix imgui origin-imgui cvlog --squash

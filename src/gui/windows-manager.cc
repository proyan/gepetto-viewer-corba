#include "gepetto/gui/windows-manager.hh"

#include <gepetto/viewer/window-manager.h>
#include <gepetto/viewer/group-node.h>

#include "gepetto/gui/osgwidget.hh"
#include "gepetto/gui/mainwindow.hh"
#include "gepetto/gui/tree-item.hh"
#include <gepetto/gui/bodytreewidget.hh>

namespace gepetto {
  namespace gui {
    WindowsManagerPtr_t WindowsManager::create(BodyTreeWidget* bodyTree)
    {
      return WindowsManagerPtr_t (new WindowsManager(bodyTree));
    }

    WindowsManager::WindowID WindowsManager::createWindow(const char *windowNameCorba)
    {
      return MainWindow::instance()->delayedCreateView(QString (windowNameCorba))->windowID();
    }

    WindowsManager::WindowID WindowsManager::createWindow(const char *windowNameCorba,
                                                          osgViewer::Viewer *viewer,
                                                          osg::GraphicsContext *gc)
    {
      std::string wn (windowNameCorba);
      graphics::WindowManagerPtr_t newWindow = graphics::WindowManager::create (viewer, gc);
      WindowID windowId = addWindow (wn, newWindow);
      return windowId;
    }

    WindowsManager::WindowsManager(BodyTreeWidget* bodyTree)
      : Parent_t ()
      , bodyTree_ (bodyTree)
    {
    }

    void WindowsManager::addNode(const std::string& nodeName, NodePtr_t node, GroupNodePtr_t parent)
    {
      Parent_t::addNode (nodeName, node, parent);
      if (parent)
        initParent(node, parent, false);
    }

    void WindowsManager::addGroup(const std::string& groupName, GroupNodePtr_t group, GroupNodePtr_t parent)
    {
      Parent_t::addGroup (groupName, group, parent);
      if (!parent || !initParent(group, parent, true)) {
        // Consider it a root group
        BodyTreeItem* bti = new BodyTreeItem (bodyTree_, group);
        nodeItemMap_[groupName].first.push_back(bti);
        nodeItemMap_[groupName].second = true;
        bodyTree_->model()->appendRow(bti);
      }
    }

    void WindowsManager::addToGroup (const std::string& nodeName, const std::string& groupName,
        const NodePtr_t& node, const BodyTreeItems_t& groups, bool isGroup)
    {
      for(std::size_t i = 0; i < groups.size(); ++i) {
        BodyTreeItem* bti = new BodyTreeItem (bodyTree_, node);
        nodeItemMap_[nodeName].first.push_back(bti);
        nodeItemMap_[nodeName].second = isGroup;
        bti->setParentGroup(groupName);
        groups[i]->appendRow(bti);
      }
    }

    bool WindowsManager::addToGroup(const std::string& nodeName, const std::string& groupName)
    {
      if (Parent_t::addToGroup(nodeName, groupName)) {
        NodePtr_t node = getNode(nodeName, false);
        bool isGroup = getGroup(nodeName, false);
        assert(node);
        BodyTreeItemMap_t::const_iterator _groups = nodeItemMap_.find(groupName);
        assert(_groups != nodeItemMap_.end());
        assert(!_groups->second.first.empty());
        assert(_groups->second.second);
        assert(getGroup(groupName));
        addToGroup(nodeName, groupName, node, _groups->second.first, isGroup);
        return true;
      }
      return false;
    }

    bool WindowsManager::removeFromGroup (const std::string& nodeName, const std::string& groupName)
    {
      bool ret = Parent_t::removeFromGroup(nodeName, groupName);
      if (ret) {
        BodyTreeItemMap_t::iterator _nodes  = nodeItemMap_.find(nodeName);
        BodyTreeItemMap_t::iterator _groups = nodeItemMap_.find(groupName);
        assert (_nodes  != nodeItemMap_.end());
        assert (_groups != nodeItemMap_.end());
        BodyTreeItems_t& nodes  = _nodes ->second.first;
        const BodyTreeItems_t& groups = _groups->second.first;
        bool found = false;
        for (BodyTreeItems_t::iterator _node = nodes.begin(); _node != nodes.end(); ++_node) {
          BodyTreeItems_t::const_iterator _group = std::find
            (groups.begin(), groups.end(), (*_node)->QStandardItem::parent());
          if (_group == groups.end()) continue;
          bodyTree_->model()->takeRow((*_node)->row());
          nodes.erase(_node);
          found = true;
          break;
        }
        if (!found)
          qDebug() << "Could not find body tree item parent" << groupName.c_str()
            << "of" << nodeName.c_str();
      }
      return ret;
    }

    bool WindowsManager::deleteNode (const std::string& nodeName, bool all)
    {
      bool ret = Parent_t::deleteNode(nodeName, all);
      if (ret) deleteBodyItem(nodeName);
      return ret;
    }

    void WindowsManager::deleteBodyItem(const std::string& nodeName)
    {
      BodyTreeItemMap_t::iterator _nodes = nodeItemMap_.find(nodeName);
      assert (_nodes != nodeItemMap_.end());
      for (std::size_t i = 0; i < _nodes->second.first.size(); ++i) {
        bodyTree_->model()->takeRow(_nodes->second.first[i]->row());
        delete _nodes->second.first[i];
        _nodes->second.first[i] = NULL;
      }
      nodeItemMap_.erase(_nodes);
    }

    bool WindowsManager::initParent (NodePtr_t node,
        GroupNodePtr_t parent, bool isGroup)
    {
      BodyTreeItemMap_t::const_iterator _groups = nodeItemMap_.find(parent->getID());
      if (_groups != nodeItemMap_.end() && _groups->second.second) {
        assert(!_groups->second.first.empty());
        addToGroup(node->getID(), parent->getID(), node, _groups->second.first, isGroup);
        return true;
      }
      return false;
    }
    }
  } // namespace gui
} // namespace gepetto

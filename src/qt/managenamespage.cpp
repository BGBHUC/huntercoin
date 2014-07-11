#include "managenamespage.h"
#include "ui_managenamespage.h"

#include <iostream>
#include <fstream>
#include "bot.h"
#include "walletmodel.h"
#include "nametablemodel.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "addressbookpage.h"
#include "../headers.h"
#include "../wallet.h"
#include "../huntercoin.h"
#include "guiconstants.h"
#include "ui_interface.h"
#include "configurenamedialog1.h"
#include "configurenamedialog2.h"
#include "gamemapview.h"
#include "../gamemap.h"
#include "gamechatview.h"
#include <boost/algorithm/string.hpp>
#include <QMessageBox>
#include <QScrollBar>
#include <QTimeLine>
#include <QInputDialog>
#include <QShortcut>

#include "../json/json_spirit.h"
#include "../json/json_spirit_writer.h"
#include "../json/json_spirit_utils.h"
#include "../json/json_spirit_writer_template.h"

extern std::map<std::vector<unsigned char>, PreparedNameFirstUpdate> mapMyNameFirstUpdate;

class CharacterTableModel : public QAbstractTableModel
{
    Q_OBJECT
    Game::PlayerID player;
    Game::PlayerState state;
    Game::CharacterID crownHolder;
    std::vector<int> alive;
    const QueuedPlayerMoves &queuedMoves;
    bool pending;      // Global flag if player transaction state is pending

public:
    CharacterTableModel(const Game::PlayerID &player_, const Game::PlayerState &state_, const QueuedPlayerMoves &queuedMoves_, Game::CharacterID crownHolder_)
        : player(player_), state(state_), queuedMoves(queuedMoves_), crownHolder(crownHolder_), pending(false)
    {
        BOOST_FOREACH(const PAIRTYPE(int, Game::CharacterState) &pc, state.characters)
            alive.push_back(pc.first);
    }

    enum ColumnIndex {
        Name = 0,
        Coins = 1,
        Status = 2,
        Time = 3,
        Life = 4,
        NUM_COLUMNS
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        Q_UNUSED(parent);
        return alive.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const
    {
        Q_UNUSED(parent);
        return NUM_COLUMNS;
    }

    void MoveQueueChanged()
    {
        emit dataChanged(index(0, Status), index(alive.size() - 1, Status));
    }

    void SetPending(bool pending_)
    {
        if (pending != pending_)
        {
            pending = pending_;
            emit dataChanged(index(0, Status), index(alive.size() - 1, Status));
        }
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
            const int i = alive[index.row()];
            QueuedPlayerMoves::const_iterator mi;
            std::map<int, Game::CharacterState>::const_iterator mi2;
            switch (index.column())
            {
                case Name:
                {
                    // TODO: for sorting return the index as number
                    Game::CharacterID chid(player, i);
                    QString ret = QString::fromStdString(chid.ToString());
                    if (role == Qt::DisplayRole)
                    {
                        if (i == 0)
                            ret = QString::fromUtf8("\u2605") + ret;
                        if (crownHolder == chid)
                            ret += QString::fromUtf8(" \u265B");
                    }
                    return ret;
                }

                case Coins:
                    mi2 = state.characters.find(i);
                    if (mi2 != state.characters.end())
                        return QString::fromStdString(FormatMoney(mi2->second.loot.nAmount));    // TODO: for sorting return as float value
                    return QVariant();

                case Status:
                    if (pending) {
                        //return tr("Pending");
                    	mi2 = state.characters.find(i);
                        if (mi2->second.waypoints.empty()) {
                        	QString ret = QString::fromStdString("Pending (");
                        	ret += QString::number(mi2->second.coord.x);
                        	ret += QString::fromStdString(",");
                        	ret += QString::number(mi2->second.coord.y);
                        	ret += QString::fromStdString(")");
                        	return ret;
                        } else {
                        	QString ret = QString::fromStdString("Pending (");
                        	ret += QString::number(mi2->second.coord.x);
                        	ret += QString::fromStdString(",");
                        	ret += QString::number(mi2->second.coord.y);
                        	ret += QString::fromStdString(") - (");
                        	ret += QString::number(mi2->second.waypoints.front().x);
                        	ret += QString::fromStdString(",");
                        	ret += QString::number(mi2->second.waypoints.front().y);
                        	ret += QString::fromStdString(")");
                        	return ret;
                        }
                    }
                    mi = queuedMoves.find(i);
                    if (mi != queuedMoves.end())
                    {
                        if (mi->second.destruct)
                            return tr("Destruct");
                        else
                            return tr("Queued");
                    }
                    else
                    {
                        mi2 = state.characters.find(i);
                        if (mi2 != state.characters.end() && mi2->second.waypoints.empty()) {
                        	QString ret = QString::fromStdString("Ready (");
                        	ret += QString::number(mi2->second.coord.x);
                        	ret += QString::fromStdString(",");
                        	ret += QString::number(mi2->second.coord.y);
                        	ret += QString::fromStdString(")");
                        	return ret;//QString::fromStdString("Ready (")+QString::number(1); //return tr("Ready" + QString::number(1));
                        } else {
                        	QString ret = QString::fromStdString("Moving (");
                        	ret += QString::number(mi2->second.coord.x);
                        	ret += QString::fromStdString(",");
                        	ret += QString::number(mi2->second.coord.y);
                        	ret += QString::fromStdString(") - (");
                        	ret += QString::number(mi2->second.waypoints.front().x);
                        	ret += QString::fromStdString(",");
                        	ret += QString::number(mi2->second.waypoints.front().y);
                        	ret += QString::fromStdString(")");
                        	return ret;
                        }
                    }
                case Time:
                  {
                    unsigned val = 0;
                    mi = queuedMoves.find(i);
                    mi2 = state.characters.find(i);
                    const Game::WaypointVector* wp = NULL;
                    if (mi != queuedMoves.end())
                        wp = &mi->second.waypoints;
                    if (mi2 != state.characters.end())
                        val = mi2->second.TimeToDestination(wp);
                    if (val > 0)
                        return QString("%1").arg(val);
                    return "";
                  }

                case Life:
                  {
                    /* Show remaining life only for general.  */
                    if (i != 0)
                      return "";

                    if (state.remainingLife == -1)
                      return "";

                    assert (state.remainingLife > 0);
                    return QString("%1").arg (state.remainingLife);
                  }
            }
        }
        else if (role == Qt::TextAlignmentRole)
            return QVariant(int(Qt::AlignLeft|Qt::AlignVCenter));

        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                    case Name:
                        return tr("Name");
                    case Coins:
                        return tr("Coins");
                    case Status:
                        return tr("Status");
                    case Time:
                        return tr("Time");
                    case Life:
                        return tr("Life");
                }
            }
            else if (role == Qt::TextAlignmentRole)
                return QVariant(int(Qt::AlignLeft|Qt::AlignVCenter));
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                    case Name:
                        return tr("Character name");
                    case Coins:
                        return tr("Amount of collected loot (return the character to spawn area to cash out)");
                    case Status:
                        return tr("Character status");
                    case Time:
                        return tr("Time until destination is reached (in blocks)");
                    case Life:
                        return tr("Remaining life in blocks");
                }
            }
        }
        return QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
    {
        Q_UNUSED(parent);
        return createIndex(row, column);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const
    {
        if (!index.isValid())
            return 0;

        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    /*@}*/

    int FindRow(const QString &name)
    {
        Game::CharacterID chid = Game::CharacterID::Parse(name.toStdString());
        if (chid.player != player)
            return -1;
        std::vector<int>::const_iterator it = std::lower_bound(alive.begin(), alive.end(), chid.index);
        if (it == alive.end())
            return -1;
        if (*it != chid.index)
            return -1;
        return it - alive.begin();
    }
};

class NameTabs : public QTabBar
{
    Q_OBJECT

public:
    NameTabs(NameTableModel *model_, QWidget *parent = 0)
        : QTabBar(parent), model(model_)
    {
#define NAME_TABS_CONNECT_HELPER(x) \
            connect(model, SIGNAL(x), this, SLOT(x))

        NAME_TABS_CONNECT_HELPER(dataChanged(const QModelIndex &, const QModelIndex &));
        NAME_TABS_CONNECT_HELPER(modelReset());
        NAME_TABS_CONNECT_HELPER(rowsInserted(const QModelIndex &, int, int));
        NAME_TABS_CONNECT_HELPER(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int));
        NAME_TABS_CONNECT_HELPER(rowsRemoved(const QModelIndex &, int, int));

#undef NAME_TABS_CONNECT_HELPER

        connect(this, SIGNAL(currentChanged(int)), this, SLOT(onCurrentChanged(int)));

        setStyleSheet("QTabBar { font-weight: bold }");

        CreateTabs();
    }

    void EmitSelect()
    {
        onCurrentChanged(currentIndex());
    }

signals:
    void onSelectName(const QString &name);
    void PendingStatusChanged();

private slots:
    void onCurrentChanged(int row)
    {
        if (row >= 0)
        {
            QModelIndex index = model->index(row, NameTableModel::Name);
            QString name = index.data(Qt::EditRole).toString();
            emit onSelectName(name);
        }
        else
            emit onSelectName(QString());
    }

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
    {
        int row_start = topLeft.row();
        int row_end = bottomRight.row();
        for (int i = row_start; i <= row_end; i++)
        {
            QModelIndex index = model->index(i, NameTableModel::Name);
            setTabText(i, index.data(Qt::DisplayRole).toString());
            setTabData(i, index.data(Qt::EditRole));
            setTabTextColor(i, index.data(Qt::DecorationRole).value<QColor>());
        }
        int cur = currentIndex();
        if (row_start <= cur && cur <= row_end)
            emit PendingStatusChanged();
    }

    void modelReset()
    {
        ClearTabs();
        CreateTabs();
    }

    void rowsInserted(const QModelIndex &parent, int start, int end)
    {
        Q_UNUSED(parent);
        for (int i = start; i <= end; i++)
        {
            QModelIndex index = model->index(i, NameTableModel::Name);
            insertTab(i, index.data(Qt::DisplayRole).toString());
            setTabTextColor(i, index.data(Qt::DecorationRole).value<QColor>());
        }
    }

    void rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
    {
        throw std::runtime_error("NameTabs::rowsMoved not implemented");
    }

    void rowsRemoved(const QModelIndex &parent, int start, int end)
    {
        Q_UNUSED(parent);
        for (int i = end; i >= start; i--)
            removeTab(i);
    }

private:
    NameTableModel *model;
    
    void ClearTabs()
    {
        for (int i = count(); i >= 0; i--)
            removeTab(i);
    }

    void CreateTabs()
    {
        for (int i = 0, n = model->rowCount(); i < n; i++)
        {
            QModelIndex index = model->index(i, NameTableModel::Name);
            addTab(index.data(Qt::DisplayRole).toString());
            setTabTextColor(i, index.data(Qt::DecorationRole).value<QColor>());
        }
    }
};

#include "managenamespage.moc"

// Prefix that indicates for the user that the player's reward address isn't the one shown
// (currently this column is hidden though)
const QString NON_REWARD_ADDRESS_PREFIX = "-";

//
// ManageNamesPage
//

const static int COLUMN_WIDTH_COINS = 70;
const static int COLUMN_WIDTH_STATE = 50;
const static int COLUMN_WIDTH_TIME = 40;
const static int COLUMN_WIDTH_LIFE = 40;

////////////////////////////////////////////////////////////////////////////
// BOTS BEGIN //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
void ManageNamesPage::loadBotConfig() {
	botConfig = BotHelper::getConfig();
	std::vector<Bot> newBots;
	for (int i = 0; i < botConfig.dests.size(); i++) {
		Bot bot;
		for (int b = 0; b < bots.size(); b++) {
			if (bots[b].name == botConfig.names[i]) {
				bot = bots[b];
				break;
			}
		}
		bot.debug = botConfig.debug;
		bot.mode = botConfig.mode;
		bot.name = botConfig.names[i];
		bot.dests = botConfig.dests[i];
		bot.botConfig = botConfig;
		bot.maxLootDistanceAtDest = botConfig.maxLootDistanceAtDest;
		bot.maxLootDistanceInRoute = botConfig.maxLootDistanceInRoute;
		bot.maxLoot = botConfig.maxLoot;
		if (botConfig.maxMaxLoot < botConfig.maxLoot) {
			bot.maxMaxLoot = botConfig.maxLoot;
		} else {
			bot.maxMaxLoot = botConfig.maxMaxLoot;
		}
		bot.stickToDest = botConfig.stickToDest;
		newBots.push_back(bot);
	}
	std::swap(bots,newBots);
}
////////////////////////////////////////////////////////////////////////////
// BOTS END //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


ManageNamesPage::ManageNamesPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ManageNamesPage),
    model(0),
    walletModel(0),
    characterTableModel(0),
    chrononAnim(0),
    tabsNames(NULL),
    rewardAddrChanged(false)
{


	////////////////////////////////////////////////////////////////////////////
	// BOTS BEGIN //////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	printf("@@@@@@ INIT BOTS \n");
	botLoadTime = time(NULL);
	ManageNamesPage::loadBotConfig();
	////////////////////////////////////////////////////////////////////////////
	// BOTS END ////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////

    ui->setupUi(this);

    connect(ui->tableCharacters, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onClickedCharacter(QModelIndex)));
    ui->tableCharacters->setEditTriggers(QAbstractItemView::NoEditTriggers);

    gameMapView = new GameMapView(this);
    ui->verticalLayoutGameMap->addWidget(gameMapView);
    gameMapView->setAttribute(Qt::WA_NoMousePropagation, true);
    gameMapView->setStatusTip(tr("Left click - make move. Right button - scroll map. Mouse wheel - zoom map. Middle click - reset zoom. Ctrl + +,-,0 - zoom in/out/reset"));

    connect(gameMapView,SIGNAL(tileHover(int,int)),this,SLOT(onTileHover(int,int)));
    connect(gameMapView, SIGNAL(tileClicked(int, int, bool)), this, SLOT(onTileClicked(int, int, bool)));

    QShortcut *zoomInKey = new QShortcut(QKeySequence::ZoomIn, this);
    QShortcut *zoomInKey2 = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Equal), this);
    QShortcut *zoomOutKey = new QShortcut(QKeySequence::ZoomOut, this);
    QShortcut *zoomResetKey = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_0), this);
    connect(zoomInKey, SIGNAL(activated()), gameMapView, SLOT(zoomIn()));
    connect(zoomInKey2, SIGNAL(activated()), gameMapView, SLOT(zoomIn()));
    connect(zoomOutKey, SIGNAL(activated()), gameMapView, SLOT(zoomOut()));
    connect(zoomResetKey, SIGNAL(activated()), gameMapView, SLOT(zoomReset()));

    gameChatView = new GameChatView(this);
    connect(gameChatView, SIGNAL(chatUpdated(const QString &)), ui->textChat, SLOT(setHtml(const QString &)));
    connect(gameChatView, SIGNAL(chatScrollToAnchor(const QString &)), ui->textChat, SLOT(scrollToAnchor(const QString &)));

    chrononAnim = new QTimeLine(3000, this);
    chrononAnim->setUpdateInterval(50);
    connect(chrononAnim, SIGNAL(valueChanged(qreal)), SLOT(chrononAnimChanged(qreal)));
    connect(chrononAnim, SIGNAL(finished()), SLOT(chrononAnimFinished()));

    onSelectName(QString());
}

ManageNamesPage::~ManageNamesPage()
{
    delete ui;
    delete characterTableModel;
}

void ManageNamesPage::setModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    model = walletModel->getNameTableModel();
    model->updateGameState();

    if (tabsNames)
        delete tabsNames;
    tabsNames = new NameTabs(model, this);
    tabsNames->setDrawBase(false);
    ui->horizontalLayoutTop->insertWidget(0, tabsNames);

    connect(tabsNames, SIGNAL(onSelectName(const QString &)), this, SLOT(onSelectName(const QString &)));
    connect(tabsNames, SIGNAL(PendingStatusChanged()), this, SLOT(PendingStatusChanged()));
    tabsNames->EmitSelect();

    connect(model, SIGNAL(gameStateChanged(const Game::GameState &)), gameChatView, SLOT(updateChat(const Game::GameState &)));
    connect(model, SIGNAL(gameStateChanged(const Game::GameState &)), this, SLOT(updateGameState(const Game::GameState &)));

    ui->tableCharacters->horizontalHeader()->setHighlightSections(false);

    model->emitGameStateChanged();
}

void ManageNamesPage::on_newButton_clicked()
{
    if (!walletModel)
        return;

    QString name;
    for (;;)
    {
        bool ok;
        name = QInputDialog::getText(this, tr("New name"), tr("Player name:"), QLineEdit::Normal, "", &ok, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
        if (!ok)
            return;

        if (!IsValidPlayerName(name.toStdString()))
            QMessageBox::warning(this, tr("Name registration error"),
                  tr("The entered name is invalid.  Allowed characters: alphanumeric, underscore, hyphen, whitespace (but not at start/end and no double whitespaces).  Maximum length is 10 characters."),
                  QMessageBox::Ok);
        else
            break;
    }

    if (!walletModel->nameAvailable(name))
    {
        QMessageBox::warning(this, tr("Name registration"), tr("Name not available"));
        return;
    }

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
        return;
    
    QString err_msg;

    try
    {
        WalletModel::NameNewReturn res = walletModel->nameNew(name);

        if (res.ok)
        {
            int newRowIndex;
            // FIXME: CT_NEW may have been sent from nameNew (via transaction).
            // Currently updateEntry is modified so it does not complain
            model->updateEntry(name, "", res.address, NameTableEntry::NAME_NEW, CT_NEW, &newRowIndex);
            tabsNames->setCurrentIndex(newRowIndex);
            onSelectName(name);

            ConfigureNameDialog1 dlg(name, std::string(), res.address, this);
            dlg.setModel(walletModel);
            if (dlg.exec() == QDialog::Accepted)
            {
                LOCK(cs_main);
                if (mapMyNameFirstUpdate.count(vchFromString(name.toStdString())) != 0)
                    model->updateEntry(name, dlg.getReturnData(), res.address, NameTableEntry::NAME_NEW, CT_UPDATED);
                else
                {
                    // name_firstupdate could have been sent, while the user was editing the value
                    // Do nothing
                }
            }

            return;
        }

        err_msg = res.err_msg;
    }
    catch (std::exception& e) 
    {
        err_msg = e.what();
    }

    if (err_msg == "ABORTED")
        return;

    QMessageBox::warning(this, tr("Name registration failed"), err_msg);
}

void ManageNamesPage::on_destructButton_clicked()
{
    if (selectedCharacters.isEmpty())
        return;

    foreach (QString character, selectedCharacters)
    {
        Game::CharacterID chid = Game::CharacterID::Parse(character.toStdString());
        if (chid.player != selectedPlayer.toStdString())
            continue;

        if (chid == gameState.crownHolder)
        {
            QMessageBox::information(this, tr("Self-destruction"),
                  tr("Crown holder cannot self-destruct"),
                  QMessageBox::Ok);
            continue;
        }

        queuedMoves[chid.player][chid.index].destruct = true;
    }    

    UpdateQueuedMoves();
}

void ManageNamesPage::on_goButton_clicked()
{
    if (selectedPlayer.isEmpty())
        return;

    if (!walletModel)
        return;

    json_spirit::Object json;

    QString msg = ui->messageEdit->text();
    if (!msg.isEmpty())
        json.push_back(json_spirit::Pair("msg", msg.toStdString()));

    // Make sure the game state is up-to-date (otherwise it's only polled every 250 ms)
    model->updateGameState();

    std::map<Game::PlayerID, Game::PlayerState>::const_iterator it = gameState.players.find(selectedPlayer.toStdString());
    if (it == gameState.players.end() || rewardAddr.toStdString() != it->second.address)
        json.push_back(json_spirit::Pair("address", rewardAddr.toStdString()));

    std::string strSelectedPlayer = selectedPlayer.toStdString();
    
    std::map<Game::PlayerID, Game::PlayerState>::const_iterator mi = gameState.players.find(strSelectedPlayer);

    const QueuedPlayerMoves &qpm = queuedMoves[strSelectedPlayer];

    BOOST_FOREACH(const PAIRTYPE(int, QueuedMove)& item, qpm)
    {
        // TODO: this can be extracted as a method QueuedMove::ToJsonValue
        json_spirit::Object obj;
        if (item.second.destruct)
            obj.push_back(json_spirit::Pair("destruct", json_spirit::Value(true)));
        else
        {
            const std::vector<Game::Coord> *p = NULL;
            if (mi != gameState.players.end())
            {
                std::map<int, Game::CharacterState>::const_iterator mi2 = mi->second.characters.find(item.first);
                if (mi2 == mi->second.characters.end())
                    continue;
                const Game::CharacterState &ch = mi2->second;

                // Caution: UpdateQueuedPath can modify the array queuedMoves that we are iterating over
                p = UpdateQueuedPath(ch, queuedMoves, Game::CharacterID(strSelectedPlayer, item.first));
            }

            if (!p || p->empty())
                p = &item.second.waypoints;

            if (p->empty())
                continue;

            json_spirit::Array arr;
            if (p->size() == 1)
            {
                // Single waypoint (which forces character to stop on the spot) is sent as is.
                // It's also possible to send an empty waypoint array for this, but the behavior will differ 
                // if it goes into the chain some blocks later (will stop on the current tile rather than
                // the clicked one).
                arr.push_back((*p)[0].x);
                arr.push_back((*p)[0].y);
            }
            else
                for (size_t i = 1, n = p->size(); i < n; i++)
                {
                    arr.push_back((*p)[i].x);
                    arr.push_back((*p)[i].y);
                }
            obj.push_back(json_spirit::Pair("wp", arr));
        }
        json.push_back(json_spirit::Pair(strprintf("%d", item.first), obj));
    }

    std::string data = json_spirit::write_string(json_spirit::Value(json), false);

    QString err_msg;
    try
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if (!ctx.isValid())
            return;

        err_msg = walletModel->nameUpdate(selectedPlayer, data, transferTo);
    }
    catch (std::exception& e) 
    {
        err_msg = e.what();
    }

    if (!err_msg.isEmpty())
    {
        if (err_msg == "ABORTED")
            return;

        printf("Name update error for player %s: %s\n\tMove: %s\n", qPrintable(selectedPlayer), qPrintable(err_msg), data.c_str());
        // BGBHUC QMessageBox::critical(this, tr("Name update error"), err_msg);
        return;
    }
    ui->messageEdit->setText(QString());
    transferTo = QString();

    queuedMoves[strSelectedPlayer].clear();

    UpdateQueuedMoves();
    SetPlayerMoveEnabled(false);
    rewardAddrChanged = false;
}

void ManageNamesPage::on_cancelButton_clicked()
{
    if (selectedCharacters.isEmpty())
        return;

    foreach (QString character, selectedCharacters)
    {
        Game::CharacterID chid = Game::CharacterID::Parse(character.toStdString());
        if (chid.player != selectedPlayer.toStdString())
            continue;

        queuedMoves[chid.player].erase(chid.index);
    }

    UpdateQueuedMoves();
}

void ManageNamesPage::UpdateQueuedMoves()
{
    if (characterTableModel)
        characterTableModel->MoveQueueChanged();
    gameMapView->SelectPlayer(selectedPlayer, gameState, queuedMoves);
}

void ManageNamesPage::onTileHover(int x, int y) {
	std::string coord = "Coord: " + BotHelper::to_string(x) + "," + BotHelper::to_string(y);
    ui->gameCoord->setText(coord.c_str());
}

void ManageNamesPage::onTileClicked(int x, int y, bool ctrlPressed)
{
    if (selectedCharacters.isEmpty())
        return;

    Game::Coord target(x, y);
    foreach (QString character, selectedCharacters)
    {
        Game::CharacterID chid = Game::CharacterID::Parse(character.toStdString());
        if (chid.player != selectedPlayer.toStdString())
            continue;

        std::map<Game::PlayerID, Game::PlayerState>::const_iterator mi = gameState.players.find(chid.player);
        if (mi == gameState.players.end())
            continue;

        std::map<int, Game::CharacterState>::const_iterator mi2 = mi->second.characters.find(chid.index);
        if (mi2 == mi->second.characters.end())
            continue;

        Game::WaypointVector &cwp = queuedMoves[chid.player][chid.index].waypoints;
        bool appendWP = (ctrlPressed && !cwp.empty());
        Game::Coord start = (appendWP) ? cwp.back() : mi2->second.coord;
        Game::WaypointVector wp = FindPath(start, target);
        
        if (wp.empty()) 
            continue;
            
        if (appendWP)
        {
            assert (wp.front () == cwp.back ());
            cwp.reserve((cwp.size() + wp.size()) - 1);
            cwp.insert(cwp.end(), wp.begin() + 1, wp.end());
        }
        else 
            cwp = wp;
    }
    UpdateQueuedMoves();
}

void ManageNamesPage::onClickedCharacter(const QModelIndex &index)
{
    QString selectedCharacter = index.sibling(index.row(), CharacterTableModel::Name).data(Qt::EditRole).toString();

    Game::CharacterID chid = Game::CharacterID::Parse(selectedCharacter.toStdString());
    
    std::map<Game::PlayerID, Game::PlayerState>::const_iterator mi = gameState.players.find(chid.player);
    if (mi != gameState.players.end())
    {
        std::map<int, Game::CharacterState>::const_iterator mi2 = mi->second.characters.find(chid.index);
        if (mi2 != mi->second.characters.end())
            gameMapView->CenterMapOnCharacter(mi2->second);
    }
}

void ManageNamesPage::RefreshCharacterList()
{
    if (characterTableModel)
        characterTableModel->deleteLater();

    std::map<Game::PlayerID, Game::PlayerState>::const_iterator it = gameState.players.find(selectedPlayer.toStdString());
    if (it != gameState.players.end())
    {
        // Note: pointer to queuedMoves is saved and must stay valid while the character table is visible
        characterTableModel = new CharacterTableModel(it->first, it->second, queuedMoves[it->first], gameState.crownHolder);
    }
    else
        characterTableModel = NULL;

    // Delete old selection model, because setModel creates a new one
    if (ui->tableCharacters->selectionModel())
        ui->tableCharacters->selectionModel()->deleteLater();
    ui->tableCharacters->setModel(characterTableModel);

    if (!characterTableModel)
    {
        gameMapView->DeselectPlayer();
        return;
    }

    connect(ui->tableCharacters->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(onCharacterSelectionChanged(QItemSelection, QItemSelection)));

    // Set column widths
    ui->tableCharacters->horizontalHeader()->resizeSection(CharacterTableModel::Coins, COLUMN_WIDTH_COINS);
    ui->tableCharacters->horizontalHeader()->resizeSection(CharacterTableModel::Status, COLUMN_WIDTH_STATE);
    ui->tableCharacters->horizontalHeader()->resizeSection(CharacterTableModel::Time, COLUMN_WIDTH_TIME);
    ui->tableCharacters->horizontalHeader()->resizeSection(CharacterTableModel::Life, COLUMN_WIDTH_LIFE);
    ui->tableCharacters->horizontalHeader()->setResizeMode(CharacterTableModel::Name, QHeaderView::Stretch);
    ui->tableCharacters->horizontalHeader()->setResizeMode(CharacterTableModel::Coins, QHeaderView::Fixed);
    ui->tableCharacters->horizontalHeader()->setResizeMode(CharacterTableModel::Status, QHeaderView::Stretch);
    ui->tableCharacters->horizontalHeader()->setResizeMode(CharacterTableModel::Time, QHeaderView::Fixed);
    ui->tableCharacters->horizontalHeader()->setResizeMode(CharacterTableModel::Life, QHeaderView::Fixed);
    QItemSelection sel;
    foreach (QString character, selectedCharacters)
    {
        int row = characterTableModel->FindRow(character);
        if (row >= 0)
            sel.select(characterTableModel->index(row, 0), characterTableModel->index(row, 1));
    }
    ui->tableCharacters->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);

    gameMapView->SelectPlayer(selectedPlayer, gameState, queuedMoves);
}

void ManageNamesPage::SetPlayerEnabled(bool enable)
{
    ui->tableCharacters->setEnabled(enable);
    ui->configButton->setEnabled(enable);
    ui->destructButton->setEnabled(enable);
    ui->cancelButton->setEnabled(enable);
    ui->messageEdit->setEnabled(enable);

    if (model)
        model->updateGameState();
    SetPlayerMoveEnabled(enable);
}

void ManageNamesPage::SetPlayerMoveEnabled(bool enable /* = true */)
{
    if (!model)
        enable = false;

    if (enable)
    {
        // If there are pending operations on the name, disable buttons
        int row = tabsNames->currentIndex();
        if (row >= 0)
            enable = model->index(row, NameTableModel::Status).data(Qt::CheckStateRole).toBool();
        else
            enable = false;
    }

    ui->goButton->setEnabled(enable);
    if (characterTableModel)
        characterTableModel->SetPending(!enable);
}

void ManageNamesPage::PendingStatusChanged()
{
    if (!selectedPlayer.isEmpty())
        SetPlayerEnabled(true);
}

void ManageNamesPage::onSelectName(const QString &name)
{
    selectedPlayer = name;
        
    rewardAddrChanged = false;

    if (selectedPlayer.isEmpty())
    {
        selectedPlayer = QString();
        transferTo = QString();
        rewardAddr = QString();
        rewardAddrChanged = false;
        ui->messageEdit->setText(QString());
        SetPlayerEnabled(false);
        return;
    }

    transferTo = QString();
    ui->messageEdit->setText(QString());

    std::map<Game::PlayerID, Game::PlayerState>::const_iterator it = gameState.players.find(selectedPlayer.toStdString());
    if (it != gameState.players.end())
        rewardAddr = QString::fromStdString(it->second.address);
    else
        rewardAddr = QString();

    selectedCharacters.clear();
    RefreshCharacterList();
    SetPlayerEnabled(true);
}

void ManageNamesPage::onCharacterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    foreach (QModelIndex index, deselected.indexes())
    {
        QString deselectedCharacter = index.sibling(index.row(), CharacterTableModel::Name).data(Qt::EditRole).toString();
        selectedCharacters.removeAll(deselectedCharacter);
    }

    foreach (QModelIndex index, selected.indexes())
    {
        QString selectedCharacter = index.sibling(index.row(), CharacterTableModel::Name).data(Qt::EditRole).toString();
        if (selectedCharacters.contains(selectedCharacter))
            continue;

        selectedCharacters.append(selectedCharacter);
    }
}

void ManageNamesPage::updateGameState(const Game::GameState &gameState)
{
    chrononAnim->stop();

    this->gameState = gameState;

    // Update reward address from the game state, unless it was explicitly changed by the user and not yet committed (via Go button)
    if (!selectedPlayer.isEmpty() && !rewardAddrChanged)
    {
        std::map<Game::PlayerID, Game::PlayerState>::const_iterator it = gameState.players.find(selectedPlayer.toStdString());
        if (it != gameState.players.end())
            rewardAddr = QString::fromStdString(it->second.address);
        else
            rewardAddr = QString();
    }

    chrononAnim->start();
    gameMapView->updateGameMap(gameState);
    RefreshCharacterList();
    SetPlayerMoveEnabled();

    ////////////////////////////////////////////////////////////////////////////////
    // BEGIN BOT CHANGES
    ////////////////////////////////////////////////////////////////////////////////
    int block = gameState.nHeight;
    printf("#####################################################################################\n");
    printf("# %d BOTS BEGIN #################  %s  #####################\n",block,FormatMoney(walletModel->getBalance()).c_str());
    printf("#####################################################################################\n");
    if (true || block % 2 == 0) {
    	ManageNamesPage::loadBotConfig();
    }

    int moveCount = 0;
	int createCounter = 0;
	int numberOfGenerals = 0;
	int numberOfPending = 0;
	int numberOfCharacters = 0;
	int64 totalLoot = 0;

	if (botConfig.enabled == 0) {
		printf("BOTS DISABLED - enabled = 0\n");
		return;
	}

	if (time(NULL)-botLoadTime < botConfig.startDelay) {
		printf("BOTS DISABLED - startDelay=%d   RUNNING %d\n",botConfig.startDelay,time(NULL)-botLoadTime);
		return;
	}

	if (block < botConfig.startBlock) {
		printf("BOTS DISABLED - startBlock=%d   WAITING FOR %d\n",botConfig.startBlock,block);
		return;
	}

    if (bots.size() == 0) {
    	printf("NO BOTS \n");
    	if (botConfig.transfer == "") {
    		return;
    	}
    }

    int64 nStart = GetTimeMillis();

    std::map<std::string,std::string> pendingPlayers;
    json_spirit::Array junk;
    json_spirit::Value pendings = name_pending(junk,false);
    json_spirit::Array pendingsArr = pendings.get_array();
    for (int p = 0; p < pendingsArr.size(); p++) {
    	json_spirit::Object pendingObj = pendingsArr[p].get_obj();
    	json_spirit::Pair pair2 = pendingObj[4];
    	if (pair2.value_.get_bool()) {
    		numberOfPending++;
    		json_spirit::Pair pair = pendingObj[0];
    		json_spirit::Pair pair3 = pendingObj[1];
    		json_spirit::Pair pair4 = pendingObj[2];
    		json_spirit::Pair pair5 = pendingObj[3];
    		pendingPlayers[pair.value_.get_str()] = pair3.value_.get_str();
    		if (botConfig.debug == 1) printf("PENDING NAME: %s %s %s %s\n",pair.value_.get_str().c_str(),pair4.value_.get_str().c_str(),pair3.value_.get_str().c_str(),pair5.value_.get_str().c_str());
    	}
    }

	if (botConfig.transfer != "") {
		botConfig.maxCreate = 0;
		int transferCount = 0;
		for (int j = 0; j < tabsNames->count(); j++) {
			if (moveCount < botConfig.maxMoves) {
				std::string tabName = tabsNames->tabText(j).toStdString();
				if (pendingPlayers.find(tabName) == pendingPlayers.end()) {
					printf("TRANSFER %s -- ",tabName.c_str());
					if (ManageNamesPage::moveBot(tabName,botConfig.transfer)) {
						moveCount++;
					}
				}
			} else {
				transferCount++;
			}
		}
		if (botConfig.debug) printf("TRANSFERS REMAINING: %d\n",transferCount);
		return;
	}

	typedef std::map<std::string,BotTarget>::iterator it_type;
	for(it_type iterator = myTargets.begin(); iterator != myTargets.end(); iterator++) {
		BotTarget bt = iterator->second;
		if (bt.removeNextBlock) {
			if (botConfig.debug == 1) printf("CLEAR TARGET VIA REMOVE FLAG %s -- ",bt.targetName.c_str());
			myTargets.erase(iterator->first);
		}
	}

	for (int botIdx = 0; botIdx < bots.size(); botIdx++) {
		Bot &bot = bots[botIdx];
		if (botConfig.debug == 1) printf("~~~~~~ %d CHECKING PLAYER %s %s BORN: %d -- ",gameState.nHeight,bot.name.c_str(),bot.mode.c_str(),bot.born);

		int botStatus = ManageNamesPage::getBotStatus(bot.name);
		if (botStatus == 1) {
			for(it_type iterator = myTargets.begin(); iterator != myTargets.end(); iterator++) {
				BotTarget bt = iterator->second;
				if (bt.hunterName == bot.name) {
					if (botConfig.debug == 1) printf("CLEAR TARGET VIA PLAYER GONE %s -- ",bt.targetName.c_str());
					myTargets.erase(iterator->first);
				}
			}
		}

		std::map<Game::PlayerID, Game::PlayerState>::const_iterator mi = gameState.players.find(bot.name);

		if (mi != gameState.players.end()) {
			numberOfGenerals++;
			BOOST_FOREACH(const PAIRTYPE(int, Game::CharacterState) &charState, mi->second.characters) {
				totalLoot += charState.second.loot.nAmount;
				if (charState.first != 0) {
					numberOfCharacters++;
				}
			}
		}

		if (pendingPlayers.find(bot.name) != pendingPlayers.end()) {
			bot.pendingCount++;
			if (botConfig.debug == 1) printf("----- PENDING %d ---- ",bot.pendingCount);
			if (bot.pendingCount > 20) {
				if (botConfig.debug == 1) printf("DELETING TRANSACTION %s -- ",pendingPlayers[bot.name].c_str());
				QString retMsg;
				bot.pendingCount = 0;
				walletModel->DeleteTransaction(QString::fromStdString(pendingPlayers[bot.name]),retMsg);
			}
			if (botConfig.debug == 1) printf("\n");
			continue;
		}

		if (mi == gameState.players.end()) {
			if (botStatus == 2) {
				if (botConfig.debug == 1) printf("NAME PENDING -- ");
			} else {
				if (botConfig.debug == 1) printf("TRY TO CREATE PLAYER -- ");
				if (createCounter < botConfig.maxCreate && (botConfig.createOnBlock == 0 || block % botConfig.createOnBlock == 0)) {
					if (bot.born < botConfig.maxBorn || botConfig.maxBorn == 0) {
						if (walletModel->getBalance() < botConfig.minCreateBalance) {
							if (botConfig.debug == 1) printf("!!!! NOT ENOUGH HUC");
						} else {
							bool  isValid = true;
							for (int d = 0; d < bot.dests.size(); d++) {
								if (!Game::IsWalkable(bot.dests[d].x,bot.dests[d].y)) {
									isValid = false;
									printf("!!!!! INVALID DEST %s -- ",BotHelper::coordToString(bot.dests[d]).c_str());
									break;
								}
							}
							if (isValid && ManageNamesPage::createBot(bot.name,botConfig.colors[botIdx])) {
								if (botConfig.debug == 1) printf("PLAYER CREATED ");
								bot.born++;
								bot.pendingCount = 0;
								createCounter++;
							} else {
								if (botConfig.debug == 1) printf("!!!!! ERROR CREATING PLAYER");
							}
						}
					} else {
						if (botConfig.debug == 1) printf("BORN TO MANY TIMES %d -- ",bot.born);
					}
				}
			}
			if (botConfig.debug == 1) printf("\n");
			continue;
		}

		bool onTab = false;
		for (int j = 0; j < tabsNames->count(); j++) {
			if (tabsNames->tabText(j).toStdString() == bot.name) {
				onTab = true;
				break;
			}
		}
		if (!onTab) {
			if (botConfig.debug == 1) printf("!!!!! SOMEBODY ELSE USING NAME\n");
			continue;
		}

		bot.pendingCount = 0;

		if (moveCount >= botConfig.maxMoves) {
			bool priority = false;
			if (botConfig.debug == 1) printf("MAX MOVES --");
			BOOST_FOREACH(const PAIRTYPE(int, Game::CharacterState) &charState, mi->second.characters) {
				if (BotHelper::coordIsSpawn(charState.second.coord)) {
					if (botConfig.debug == 1) printf("PRIORITY MOVE --");
					priority = true;
				}
			}
			if (!priority) {
				if (botConfig.debug == 1) printf("\n");
				continue;
			}
		}

		if (bot.born == 0) {
			bot.born = 1;
		}
		std::map<int,BotMove> botMoves = bot.calculate(mi->second,gameState,myTargets);
		bool executeMove = false;
		BOOST_FOREACH(const PAIRTYPE(int, Game::CharacterState) &charState, mi->second.characters) {
			if (botMoves.find(charState.first) != botMoves.end()) {
				if (botMoves[charState.first].destruct) {
					if (botConfig.debug) printf("DESTRUCT MOVE FOR CHAR %d -- ",charState.first);
					queuedMoves[bot.name][charState.first].destruct = true;
					executeMove = true;
				}
				if (botMoves[charState.first].coords.size() != 0) {
					if (botConfig.debug) {
						printf("WAYPOINTS FOR CHAR %d -- ",charState.first);
						for (int i = 0; i < botMoves[charState.first].coords.size(); i++) {
							printf("%d,%d ",botMoves[charState.first].coords[i].x,botMoves[charState.first].coords[i].y);
						}
						printf("-- ");
					}


			        Game::WaypointVector &cwp = queuedMoves[bot.name][charState.first].waypoints;
			        cwp.clear();
					Game::Coord start = charState.second.coord;
					bool validCoord = true;
					for (int i = 0; i < botMoves[charState.first].coords.size(); i++) {
						Game::Coord target = botMoves[charState.first].coords[i];
						Game::WaypointVector wp = FindPath(start,target);
						if (wp.empty()) {
							printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! INVALID COORDS %d,%d -- ",target.x,target.y);
							validCoord = false;
							break;
						}
						if (i != 0) {
				            assert (wp.front () == cwp.back ());
				            cwp.reserve((cwp.size() + wp.size()) - 1);
				            cwp.insert(cwp.end(), wp.begin() + 1, wp.end());
						} else {
							cwp = wp;
						}
						start = cwp.back();
					}
					if (validCoord) {
						if (botConfig.debug == 1) printf("YAY MOVE IT FROM (%d,%d) TO (%d,%d) -- ",charState.second.coord.x,charState.second.coord.y,cwp.back().x,cwp.back().y);
						executeMove = true;
						if (!charState.second.waypoints.empty() && charState.second.waypoints.front() == cwp.back()) {
							if (botConfig.debug == 1) printf("INVALIDATE MOVE -- ");
							executeMove = false;
						}
					}
				}
			} else {
				if (botConfig.debug) printf("NO MOVE FOR CHAR %d -- ",charState.first);
			}
		}
		if (executeMove) {
			if (ManageNamesPage::moveBot(bot.name,"")) {
				moveCount++;
			}
		}
		if (botConfig.debug == 1) printf("\n");
	}
	printf("\n#################################################################################################################\n");
	printf("SUMMARY:  %s Block: %d   Generals: %d   Characters: %d  Loot: %s   Moves: %d   Pending: %d  Took: %"PRI64d"ms \n",bots[0].name.c_str(),gameState.nHeight,numberOfGenerals,numberOfCharacters,FormatMoney(totalLoot).c_str(),moveCount,numberOfPending,GetTimeMillis() - nStart);
	printf("###################################################################################################################\n");
	////////////////////////////////////////////////////////////////////////////////////////////////
	// END BOT CHANGES
	////////////////////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////////////////////
// BEGIN BOT CHANGES
////////////////////////////////////////////////////////////////////////////////////////////////
int ManageNamesPage::getBotStatus(std::string playerName) {
	// return 0 = ready to go
	// return 1 = DNE
	// return 2 = Pending
	for (int j = 0; j < tabsNames->count(); j++) {
		std::string tabName = tabsNames->tabText(j).toStdString();
		if (tabName == playerName) {
			if (!model->index(j, NameTableModel::Status).data(Qt::CheckStateRole).toBool()) {
				return 2;
			}
			return 0;
		}
	}
	return 1;
}
bool ManageNamesPage::moveBot(std::string playerName,std::string transfer) {
	json_spirit::Object json;
    model->updateGameState();
    std::map<Game::PlayerID, Game::PlayerState>::const_iterator mi = gameState.players.find(playerName);
    const QueuedPlayerMoves &qpm = queuedMoves[playerName];
    BOOST_FOREACH(const PAIRTYPE(int, QueuedMove)& item, qpm)
    {
        // TODO: this can be extracted as a method QueuedMove::ToJsonValue
        json_spirit::Object obj;
        if (item.second.destruct)
            obj.push_back(json_spirit::Pair("destruct", json_spirit::Value(true)));
        else
        {
            const std::vector<Game::Coord> *p = NULL;
            if (mi != gameState.players.end())
            {
                std::map<int, Game::CharacterState>::const_iterator mi2 = mi->second.characters.find(item.first);
                if (mi2 == mi->second.characters.end())
                    continue;
                const Game::CharacterState &ch = mi2->second;
                // Caution: UpdateQueuedPath can modify the array queuedMoves that we are iterating over
                p = UpdateQueuedPath(ch, queuedMoves, Game::CharacterID(playerName, item.first));
            }
            if (!p || p->empty())
                p = &item.second.waypoints;
            if (p->empty())
                continue;
            json_spirit::Array arr;
            if (p->size() == 1)
            {
                // Single waypoint (which forces character to stop on the spot) is sent as is.
                // It's also possible to send an empty waypoint array for this, but the behavior will differ
                // if it goes into the chain some blocks later (will stop on the current tile rather than
                // the clicked one).
                arr.push_back((*p)[0].x);
                arr.push_back((*p)[0].y);
            }
            else
                for (size_t i = 1, n = p->size(); i < n; i++)
                {
                    arr.push_back((*p)[i].x);
                    arr.push_back((*p)[i].y);
                }
            obj.push_back(json_spirit::Pair("wp", arr));
        }
        json.push_back(json_spirit::Pair(strprintf("%d", item.first), obj));
    }
    std::string data = json_spirit::write_string(json_spirit::Value(json), false);
    if (botConfig.debug) printf("%s -- ",data.c_str());
    QString err_msg;
    try
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if (!ctx.isValid())
            return false;

        err_msg = walletModel->nameUpdate(QString::fromStdString(playerName), data, QString::fromStdString(transfer));
    }
    catch (std::exception& e)
    {
        err_msg = e.what();
    }
    if (!err_msg.isEmpty())
    {
        if (err_msg == "ABORTED")
            return false;

        printf("Name update error for player %s: %s\n\tMove: %s\n", qPrintable(QString::fromStdString(playerName)), qPrintable(err_msg), data.c_str());
        //QMessageBox::critical(this, tr("Name update error"), err_msg);
        return false;
    }
    transferTo = QString();
    queuedMoves[playerName].clear();
    UpdateQueuedMoves();
    SetPlayerMoveEnabled(false);
    rewardAddrChanged = false;
    return true;
}
bool ManageNamesPage::createBot(std::string playerName,int color) {
	if (walletModel->nameAvailable(QString::fromStdString(playerName))) {
		try {
			WalletModel::NameNewReturn res = walletModel->nameNew(QString::fromStdString(playerName));
			if (res.ok)  {
				int newRowIndex;
				model->updateEntry(QString::fromStdString(playerName), "", res.address, NameTableEntry::NAME_NEW, CT_NEW, &newRowIndex);
				json_spirit::Object obj;
				obj.push_back(json_spirit::Pair("color",color));
				json_spirit::Value val(obj);
				walletModel->nameFirstUpdatePrepare(QString::fromStdString(playerName), json_spirit::write_string(val, false), false);
				LOCK(cs_main);
				if (mapMyNameFirstUpdate.count(vchFromString(playerName)) != 0) {
					model->updateEntry(QString::fromStdString(playerName), json_spirit::write_string(val, false), res.address, NameTableEntry::NAME_NEW, CT_UPDATED);
					return true;
				} else {
					return false;
				}
			}
		} catch (std::exception& e)	{
			return false;
		}
	} else {
		return false;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////
// END BOT CHANGES
////////////////////////////////////////////////////////////////////////////////////////////////




void ManageNamesPage::chrononAnimChanged(qreal t)
{
    const static qreal FADE_IN = qreal(0.35);
    if (t <= FADE_IN)
        t /= FADE_IN;
    else
        t = 1 - (t - FADE_IN) / (1 - FADE_IN);
    QColor c(int(t * 255), 0, 0);

    ui->chrononLabel->setText(
            "<font color='" + c.name() + "'><b>"
            + tr("Chronon: %1").arg(gameState.nHeight)
            + QString("</b></font>")
        );
}

void ManageNamesPage::chrononAnimFinished()
{
    ui->chrononLabel->setText(tr("Chronon: %1").arg(gameState.nHeight));
}

void ManageNamesPage::on_configButton_clicked()
{
    int row = tabsNames->currentIndex();
    if (row < 0)
        return;

    QString name = model->index(row, NameTableModel::Name).data(Qt::EditRole).toString();
    QString address = model->index(row, NameTableModel::Address).data(Qt::EditRole).toString();
    QByteArray raw_value = model->index(row, NameTableModel::Value).data(NameTableModel::RawStringData).toByteArray();
    std::string value(raw_value.constData(), raw_value.length());

    if (address.startsWith(NON_REWARD_ADDRESS_PREFIX))
        address = address.mid(NON_REWARD_ADDRESS_PREFIX.size());

    std::vector<unsigned char> vchName = vchFromString(name.toStdString());

    bool fOldPostponed;

    {
        LOCK(cs_main);
        if (mapMyNameFirstUpdate.count(vchName) != 0)
        {
            // Postpone the firstupdate, while the dialog is open
            // If OK is pressed, fPostponed is always set to false
            // If Cancel is pressed, we restore the old fPostponed value
            fOldPostponed = mapMyNameFirstUpdate[vchName].fPostponed;
            mapMyNameFirstUpdate[vchName].fPostponed = true;
        }
        else
        {
            ConfigureNameDialog2 dlg(name, address, rewardAddr, transferTo, this);
            dlg.setModel(walletModel);
            if (dlg.exec() == QDialog::Accepted)
            {
                rewardAddr = dlg.getRewardAddr();
                rewardAddrChanged = true;
                transferTo = dlg.getTransferTo();
                QMessageBox::information(this, tr("Name configured"), tr("To apply changes, you need to press Go button"));
            }
            return;
        }
    }

    ConfigureNameDialog1 dlg(name, value, address, this);
    dlg.setModel(walletModel);
    int dlgRes = dlg.exec();

    if (dlgRes == QDialog::Accepted)
    {
        LOCK(cs_main);
        // name_firstupdate could have been sent, while the user was editing the value
        if (mapMyNameFirstUpdate.count(vchName) != 0)
            model->updateEntry(name, dlg.getReturnData(), address, NameTableEntry::NAME_NEW, CT_UPDATED);
    }
    else
    {
        LOCK(cs_main);
        // If cancel was pressed, restore the old fPostponed value
        if (mapMyNameFirstUpdate.count(vchName) != 0)
            mapMyNameFirstUpdate[vchName].fPostponed = fOldPostponed;
    }
}

void ManageNamesPage::exportClicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Registered Names Data"), QString(),
            tr("Comma separated file (*.csv)"));

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    writer.setModel(model);
    // name, column, role
    writer.addColumn("Name", NameTableModel::Name, Qt::EditRole);
    writer.addColumn("Last Move", NameTableModel::Value, Qt::EditRole);
    writer.addColumn("Address", NameTableModel::Address, Qt::EditRole);
    writer.addColumn("State", NameTableModel::State, Qt::EditRole);
    writer.addColumn("Status", NameTableModel::Status, Qt::EditRole);

    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}

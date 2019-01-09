/*
 * Project: curve
 * Created Date: Wednesday September 12th 2018
 * Author: hzsunjianliang
 * Copyright (c) 2018 netease
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/mds/nameserver2/curvefs.h"
#include "src/mds/nameserver2/inode_id_generator.h"
#include "src/mds/nameserver2/namespace_storage.h"
#include "test/mds/nameserver2/mock_namespace_storage.h"
#include "test/mds/nameserver2/mock_inode_id_generator.h"
#include "test/mds/nameserver2/mock_chunk_allocate.h"
#include "test/mds/nameserver2/mock_clean_manager.h"
#include "src/mds/common/topology_define.h"

using ::testing::AtLeast;
using ::testing::StrEq;
using ::testing::_;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::DoAll;
using ::testing::SetArgPointee;

namespace curve {
namespace mds {

class CurveFSTest: public ::testing::Test {
 protected:
    void SetUp() override {
        storage_ = new MockNameServerStorage();
        inodeIdGenerator_ = new MockInodeIDGenerator();
        mockChunkAllocator_ = new MockChunkAllocator();
        mockSnapShotCleanManager_ = std::make_shared<MockCleanManager>();

        curvefs_ =  &kCurveFS;
        curvefs_->Init(storage_, inodeIdGenerator_,
            mockChunkAllocator_, mockSnapShotCleanManager_);
    }

    void TearDown() override {
        delete storage_;
        delete inodeIdGenerator_;
        delete mockChunkAllocator_;
    }

    CurveFS *curvefs_;
    MockNameServerStorage *storage_;
    MockInodeIDGenerator *inodeIdGenerator_;
    MockChunkAllocator *mockChunkAllocator_;
    std::shared_ptr<MockCleanManager> mockSnapShotCleanManager_;
};

TEST_F(CurveFSTest, testCreateFile1) {
    // test parm error
    ASSERT_EQ(curvefs_->CreateFile("/file1", FileType::INODE_PAGEFILE,
                                   kMiniFileLength-1), StatusCode::kParaError);

    ASSERT_EQ(curvefs_->CreateFile("/", FileType::INODE_DIRECTORY, 0),
              StatusCode::kFileExists);

    {
        // test file exist
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::OK));

        auto statusCode = curvefs_->CreateFile("/file1",
                    FileType::INODE_PAGEFILE, kMiniFileLength);
        ASSERT_EQ(statusCode, StatusCode::kFileExists);
    }

    {
        // test get storage error
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::InternalError));

        auto statusCode = curvefs_->CreateFile("/file1",
                    FileType::INODE_PAGEFILE, kMiniFileLength);
        ASSERT_EQ(statusCode, StatusCode::kStorageError);
    }

    {
        // test put storage error
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::KeyNotExist));

        EXPECT_CALL(*storage_, PutFile(_, _))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::InternalError));

        EXPECT_CALL(*inodeIdGenerator_, GenInodeID(_))
        .Times(1)
        .WillOnce(Return(true));

        auto statusCode = curvefs_->CreateFile("/file1",
                    FileType::INODE_PAGEFILE, kMiniFileLength);
        ASSERT_EQ(statusCode, StatusCode::kStorageError);
    }

    {
        // test put storage ok
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::KeyNotExist));

        EXPECT_CALL(*storage_, PutFile(_, _))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::OK));

        EXPECT_CALL(*inodeIdGenerator_, GenInodeID(_))
        .Times(1)
        .WillOnce(Return(true));


        auto statusCode = curvefs_->CreateFile("/file1",
            FileType::INODE_PAGEFILE, kMiniFileLength);
        ASSERT_EQ(statusCode, StatusCode::kOK);
    }

    {
        // test inode allocate error
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::KeyNotExist));

        EXPECT_CALL(*inodeIdGenerator_, GenInodeID(_))
        .Times(1)
        .WillOnce(Return(false));

        auto statusCode = curvefs_->CreateFile("/file1",
                FileType::INODE_PAGEFILE, kMiniFileLength);
        ASSERT_EQ(statusCode, StatusCode::kStorageError);
    }
}

TEST_F(CurveFSTest, testGetFileInfo) {
    // test parm error
    FileInfo fileInfo;
    auto ret = curvefs_->GetFileInfo("/", &fileInfo);
    ASSERT_EQ(ret, StatusCode::kOK);

    FileInfo rootFileInfo = curvefs_->GetRootFileInfo();
    ASSERT_EQ(fileInfo.id(), rootFileInfo.id());
    ASSERT_EQ(fileInfo.filename(),  rootFileInfo.filename());
    ASSERT_EQ(fileInfo.filetype(), rootFileInfo.filetype());

    {
        // test path not exist
        FileInfo  fileInfo;
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));
        ASSERT_EQ(curvefs_->GetFileInfo("/file1/file2", &fileInfo),
                  StatusCode::kFileNotExists);
    }
    {
        // test stoarge error
        FileInfo fileInfo;
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::InternalError));
        ASSERT_EQ(curvefs_->GetFileInfo("/file1/file2", &fileInfo),
                  StatusCode::kStorageError);
    }
    {
        // test  ok
        FileInfo fileInfo;
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillRepeatedly(Return(StoreStatus::OK));

        ASSERT_EQ(curvefs_->GetFileInfo("/file1/file2", &fileInfo),
                  StatusCode::kOK);
    }
    {
        // test  WalkPath NOT DIRECTORY
        FileInfo  fileInfo;
        fileInfo.set_filetype(FileType::INODE_PAGEFILE);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo),
            Return(StoreStatus::OK)));

        FileInfo retFileInfo;
        std::string lastEntry;
        ASSERT_EQ(curvefs_->GetFileInfo("/testdir/file1", &retFileInfo),
            StatusCode::kFileNotExists);
    }
    {
        // test LookUpFile internal Error
        FileInfo  fileInfo;
        fileInfo.set_filetype(FileType::INODE_DIRECTORY);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo),
            Return(StoreStatus::OK)))
        .WillOnce(Return(StoreStatus::InternalError));

        FileInfo fileInfo1;
        ASSERT_EQ(curvefs_->GetFileInfo("testdir/file1", &fileInfo1),
            StatusCode::kStorageError);
    }
}


TEST_F(CurveFSTest, testDeleteFile) {
    // test remove root
    ASSERT_EQ(curvefs_->DeleteFile("/"), StatusCode::kParaError);

    // test ok
    {
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(StoreStatus::OK));

        EXPECT_CALL(*storage_, DeleteFile(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(StoreStatus::OK));

        ASSERT_EQ(curvefs_->DeleteFile("/file1"), StatusCode::kOK);
    }

    //  test filenotexist fail
    {
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::KeyNotExist));

        ASSERT_EQ(curvefs_->DeleteFile("/file1"), StatusCode::kFileNotExists);
    }

    // test storage delete  error
    {
        EXPECT_CALL(*storage_, DeleteFile(_))
        .Times(AtLeast(1))
        .WillOnce(Return(StoreStatus::InternalError));

        ASSERT_EQ(curvefs_->DeleteFile("/file1"), StatusCode::kStorageError);
    }
}

TEST_F(CurveFSTest, testReadDir) {
    FileInfo fileInfo;
    std::vector<FileInfo> items;

    // test not directory
    {
        fileInfo.set_filetype(FileType::INODE_PAGEFILE);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->ReadDir("/file1", &items),
                  StatusCode::kNotDirectory);
        items.clear();
    }

    // test getFile Not exist
    {
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));

        ASSERT_EQ(curvefs_->ReadDir("/file1", &items),
                  StatusCode::kDirNotExist);
    }

    // test listFile ok
    {
        fileInfo.set_filetype(FileType::INODE_DIRECTORY);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo),
                        Return(StoreStatus::OK)));

        std::vector<FileInfo> sideEffectArgs;
        sideEffectArgs.clear();
        sideEffectArgs.push_back(fileInfo);
        fileInfo.set_filetype(FileType::INODE_PAGEFILE);
        sideEffectArgs.push_back(fileInfo);

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(sideEffectArgs),
                        Return(StoreStatus::OK)));

        auto ret = curvefs_->ReadDir("/file1", &items);
        ASSERT_EQ(ret, StatusCode::kOK);
        ASSERT_EQ(items.size(), 2);
        ASSERT_EQ(items[0].filetype(), INODE_DIRECTORY);
        ASSERT_EQ(items[1].filetype(), INODE_PAGEFILE);
    }
}


TEST_F(CurveFSTest, testRenameFile) {
    FileInfo fileInfo;

    // test rename ok
    {
        fileInfo.set_filetype(FileType::INODE_DIRECTORY);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(3))
        .WillOnce(Return(StoreStatus::OK))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo),
                        Return(StoreStatus::OK)))
        .WillOnce(Return(StoreStatus::KeyNotExist));

        EXPECT_CALL(*storage_, RenameFile(_, _, _, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        ASSERT_EQ(curvefs_->RenameFile("/file1", "/trash/file2"),
                  StatusCode::kOK);
    }

    // old file not exist
    {
        fileInfo.set_filetype(FileType::INODE_DIRECTORY);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));

        ASSERT_EQ(curvefs_->RenameFile("/file1", "/trash/file2"),
                  StatusCode::kFileNotExists);
    }

    // new file parent directory not exist
    {
        fileInfo.set_filetype(FileType::INODE_DIRECTORY);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(Return(StoreStatus::OK))
        .WillOnce(Return(StoreStatus::KeyNotExist));

        ASSERT_EQ(curvefs_->RenameFile("/file1", "/trash/file2"),
                  StatusCode::kFileNotExists);
    }

    // new file exist
    {
        fileInfo.set_filetype(FileType::INODE_DIRECTORY);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(3))
        .WillOnce(Return(StoreStatus::OK))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo),
                        Return(StoreStatus::OK)))
        .WillOnce(Return(StoreStatus::OK));

        ASSERT_EQ(curvefs_->RenameFile("/file1", "/trash/file2"),
                  StatusCode::kFileExists);
    }

    // storage renamefile fail
    {
        fileInfo.set_filetype(FileType::INODE_DIRECTORY);
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(AtLeast(3))
        .WillOnce(Return(StoreStatus::OK))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo),
                        Return(StoreStatus::OK)))
        .WillOnce(Return(StoreStatus::KeyNotExist));

        EXPECT_CALL(*storage_, RenameFile(_, _, _, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::InternalError));

        ASSERT_EQ(curvefs_->RenameFile("/file1", "/trash/file2"),
                  StatusCode::kStorageError);
    }

    // rename same file
    {
        ASSERT_EQ(curvefs_->RenameFile("/file1", "/file1"),
                  StatusCode::kFileExists);
    }
}

TEST_F(CurveFSTest, testExtendFile) {
    // test try small filesize && same
    {
        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);


        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->ExtendFile("/user1/file1", 0),
                  StatusCode::kShrinkBiggerFile);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->ExtendFile("/user1/file1",
                                       kMiniFileLength), StatusCode::kOK);
    }

    // test enlarge size unit is not segment
    {
        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->ExtendFile("/user1/file1",
            1 + kMiniFileLength), StatusCode::kExtentUnitError);
    }

    // test enlarge size ok
    {
        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, PutFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        ASSERT_EQ(curvefs_->ExtendFile("/user1/file1",
                                       2 * kMiniFileLength), StatusCode::kOK);
    }

    // file not exist
    {
        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(Return(StoreStatus::KeyNotExist));

        ASSERT_EQ(curvefs_->ExtendFile("/user1/file1",
                                       2 * kMiniFileLength),
                                       StatusCode::kFileNotExists);
    }

    // extend directory
    {
        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_DIRECTORY);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->ExtendFile("/user1/file1",
                                       2 * kMiniFileLength),
                                       StatusCode::kNotSupported);
    }
}


TEST_F(CurveFSTest, testGetOrAllocateSegment) {
    // test normal get exist segment
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        ASSERT_EQ(curvefs_->GetOrAllocateSegment("/user1/file2",
                  0, false,  &segment), StatusCode::kOK);
    }

    // test normal get & allocate not exist segment
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));


        EXPECT_CALL(*mockChunkAllocator_, AllocateChunkSegment(_, _, _, _, _))
        .Times(1)
        .WillOnce(Return(true));


        EXPECT_CALL(*storage_, PutSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        ASSERT_EQ(curvefs_->GetOrAllocateSegment("/user1/file2",
                  0, true,  &segment), StatusCode::kOK);
    }

    // file is a directory
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_DIRECTORY);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->GetOrAllocateSegment("/user1/file2",
                  0, false,  &segment), StatusCode::kParaError);
    }

    // segment offset not align file segment size
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->GetOrAllocateSegment("/user1/file2",
                  1, false,  &segment), StatusCode::kParaError);
    }

    // file length < segment offset + segmentsize
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->GetOrAllocateSegment("/user1/file2",
                  kMiniFileLength, false,  &segment), StatusCode::kParaError);
    }

    // alloc chunk segment fail
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));


        EXPECT_CALL(*mockChunkAllocator_, AllocateChunkSegment(_, _, _, _, _))
        .Times(1)
        .WillOnce(Return(false));

        ASSERT_EQ(curvefs_->GetOrAllocateSegment("/user1/file2",
                  0, true,  &segment), StatusCode::kSegmentAllocateError);
    }

    // put segment fail
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));


        EXPECT_CALL(*mockChunkAllocator_, AllocateChunkSegment(_, _, _, _, _))
        .Times(1)
        .WillOnce(Return(true));


        EXPECT_CALL(*storage_, PutSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::InternalError));

        ASSERT_EQ(curvefs_->GetOrAllocateSegment("/user1/file2",
                  0, true,  &segment), StatusCode::kStorageError);
    }
}


TEST_F(CurveFSTest, testDeleteSegment) {
    // test normal delete exist segment
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        EXPECT_CALL(*storage_, DeleteSegment(_))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        ASSERT_EQ(curvefs_->DeleteSegment("/user1/file2",
                                          0), StatusCode::kOK);
    }

    // file type 不是pagefile
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_DIRECTORY);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->DeleteSegment("/user1/file2",
                                          0), StatusCode::kParaError);
    }

    // offset not align segment size
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->DeleteSegment("/user1/file2",
                                          1), StatusCode::kParaError);
    }

    // offset + segmentsize > file length
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        ASSERT_EQ(curvefs_->DeleteSegment("/user1/file2",
                            kMiniFileLength), StatusCode::kParaError);
    }

    // get segment not exist
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));

        ASSERT_EQ(curvefs_->DeleteSegment("/user1/file2",
                                          0), StatusCode::kSegmentNotAllocated);
    }

    // delete segment fail
    {
        PageFileSegment segment;

        FileInfo fileInfo1;
        fileInfo1.set_filetype(FileType::INODE_DIRECTORY);

        FileInfo fileInfo2;
        fileInfo2.set_filetype(FileType::INODE_PAGEFILE);
        fileInfo2.set_length(kMiniFileLength);
        fileInfo2.set_segmentsize(DefaultSegmentSize);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo1),
                        Return(StoreStatus::OK)))
        .WillOnce(DoAll(SetArgPointee<1>(fileInfo2),
                        Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        EXPECT_CALL(*storage_, DeleteSegment(_))
        .Times(1)
        .WillOnce(Return(StoreStatus::InternalError));

        ASSERT_EQ(curvefs_->DeleteSegment("/user1/file2",
                                          0), StatusCode::kStorageError);
    }
}


TEST_F(CurveFSTest, testCreateSnapshotFile) {
    {
        // test under snapshot
        FileInfo originalFile;
        originalFile.set_seqnum(1);
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo fileInfo1;
        snapShotFiles.push_back(fileInfo1);

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        FileInfo snapShotFileInfoRet;
        ASSERT_EQ(curvefs_->CreateSnapShotFile("/snapshotFile1",
                &snapShotFileInfoRet), StatusCode::kFileUnderSnapShot);
    }
    {
        // test File is not PageFile
    }
    {
        // test storage ListFile error
    }
    {
        // test GenId error
    }
    {
        // test create snapshot ok
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        EXPECT_CALL(*inodeIdGenerator_, GenInodeID(_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<0>(2),
            Return(true)));

        FileInfo snapShotFileInfo;
        EXPECT_CALL(*storage_, SnapShotFile(_, _, _, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        FileInfo snapShotFileInfoRet;
        ASSERT_EQ(curvefs_->CreateSnapShotFile("/originalFile",
                &snapShotFileInfoRet), StatusCode::kOK);
        ASSERT_EQ(snapShotFileInfoRet.parentid(), originalFile.id());
        ASSERT_EQ(snapShotFileInfoRet.filename(),
            originalFile.filename() + "-" +
            std::to_string(originalFile.seqnum()) );
        ASSERT_EQ(snapShotFileInfoRet.fullpathname(),
            originalFile.fullpathname() + "/" +
            snapShotFileInfoRet.filename());
        ASSERT_EQ(snapShotFileInfoRet.filestatus(), FileStatus::kFileCreated);
    }
    {
        // test storage snapshotFile Error
    }
}

TEST_F(CurveFSTest, testListSnapShotFile) {
    {
        // workPath error
    }
    {
        // dir not support
        std::vector<FileInfo> snapFileInfos;
        ASSERT_EQ(curvefs_->ListSnapShotFile("/", &snapFileInfos),
        StatusCode::kNotSupported);
    }
    {
        // lookupFile error
        std::vector<FileInfo> snapFileInfos;
        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));

        ASSERT_EQ(curvefs_->ListSnapShotFile("/originalFile", &snapFileInfos),
            StatusCode::kFileNotExists);
    }
    {
        // check type not support
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_DIRECTORY);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapFileInfos;
        ASSERT_EQ(curvefs_->ListSnapShotFile("originalFile", &snapFileInfos),
        StatusCode::kNotSupported);
    }
    {
        // ListFile error
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::InternalError));

        std::vector<FileInfo> snapFileInfos;
        ASSERT_EQ(curvefs_->ListSnapShotFile("originalFile", &snapFileInfos),
        StatusCode::kStorageError);
    }
    {
        // ListFile ok
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapFileInfos;
        FileInfo  snapShotFile;
        snapShotFile.set_parentid(1);
        snapFileInfos.push_back(snapShotFile);

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapFileInfos),
        Return(StoreStatus::OK)));

        std::vector<FileInfo> snapFileInfosRet;
        ASSERT_EQ(curvefs_->ListSnapShotFile("originalFile", &snapFileInfosRet),
        StatusCode::kOK);

        ASSERT_EQ(snapFileInfosRet.size(), 1);
        ASSERT_EQ(snapFileInfosRet[0].SerializeAsString(),
                snapShotFile.SerializeAsString());
        }
}


TEST_F(CurveFSTest, testGetSnapShotFileInfo) {
    {
        // ListSnapShotFile error
        FileInfo snapshotFileInfo;
        ASSERT_EQ(curvefs_->GetSnapShotFileInfo("/", 1, &snapshotFileInfo),
        StatusCode::kNotSupported);
    }
    {
        // snapfile not exist(not under snapshot)
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        FileInfo snapshotFileInfo;
        ASSERT_EQ(curvefs_->GetSnapShotFileInfo("/originalFile",
            1, &snapshotFileInfo), StatusCode::kSnapshotFileNotExists);
    }
    {
        // under snapshot, butsnapfile not exist
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_seqnum(2);
        snapShotFiles.push_back(snapInfo);
        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        FileInfo snapshotFileInfo;
        ASSERT_EQ(curvefs_->GetSnapShotFileInfo("/originalFile",
            1, &snapshotFileInfo), StatusCode::kSnapshotFileNotExists);
    }
    {
        // test ok
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_seqnum(1);
        snapShotFiles.push_back(snapInfo);
        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        FileInfo snapshotFileInfo;
        ASSERT_EQ(curvefs_->GetSnapShotFileInfo("/originalFile",
            1, &snapshotFileInfo), StatusCode::kOK);
        ASSERT_EQ(snapshotFileInfo.SerializeAsString(),
        snapInfo.SerializeAsString());
    }
}

TEST_F(CurveFSTest, GetSnapShotFileSegment) {
    {
        // GetSnapShotFileInfo error
        PageFileSegment segment;
        ASSERT_EQ(curvefs_->GetSnapShotFileSegment("/", 1, 0, &segment),
            StatusCode::kNotSupported);
    }
    {
        // offset not align
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_seqnum(1);
        snapInfo.set_segmentsize(DefaultSegmentSize);
        snapShotFiles.push_back(snapInfo);
        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        PageFileSegment segment;
        ASSERT_EQ(curvefs_->GetSnapShotFileSegment("/originalFile",
            1, 1, &segment), StatusCode::kParaError);
    }
    {
        // storage->GetSegment return error
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_seqnum(1);
        snapInfo.set_segmentsize(DefaultSegmentSize);
        snapInfo.set_length(DefaultSegmentSize);
        snapShotFiles.push_back(snapInfo);
        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::KeyNotExist));

        PageFileSegment segment;
        ASSERT_EQ(curvefs_->GetSnapShotFileSegment("/originalFile",
            1, 0, &segment), StatusCode::kSegmentNotAllocated);
    }
    {
        // ok
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_seqnum(1);
        snapInfo.set_segmentsize(DefaultSegmentSize);
        snapInfo.set_length(DefaultSegmentSize);
        snapShotFiles.push_back(snapInfo);
        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        PageFileSegment expectSegment;
        expectSegment.set_logicalpoolid(1);
        expectSegment.set_segmentsize(DefaultSegmentSize);
        expectSegment.set_chunksize(DefaultChunkSize);
        expectSegment.set_startoffset(0);

        PageFileChunkInfo *chunkInfo = expectSegment.add_chunks();
        chunkInfo->set_chunkid(1);
        chunkInfo->set_copysetid(1);

        EXPECT_CALL(*storage_, GetSegment(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(expectSegment),
            Return(StoreStatus::OK)));

        PageFileSegment segment;
        ASSERT_EQ(curvefs_->GetSnapShotFileSegment("/originalFile",
            1, 0, &segment), StatusCode::kOK);
        ASSERT_EQ(expectSegment.SerializeAsString(),
                    segment.SerializeAsString());
    }
}


TEST_F(CurveFSTest, DeleteFileSnapShotFile) {
    {
        // GetSnapShotFileInfo error
        FileInfo snapshotFileInfo;
        ASSERT_EQ(curvefs_->DeleteFileSnapShotFile("/", 1, nullptr),
        StatusCode::kNotSupported);
    }
    {
        // under deleteing
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_seqnum(1);
        snapInfo.set_filestatus(FileStatus::kFileDeleting);
        snapShotFiles.push_back(snapInfo);

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        EXPECT_EQ(curvefs_->DeleteFileSnapShotFile("/originalFile", 1, nullptr),
            StatusCode::kSnapshotDeleting);
    }
    {
        // delete snapshot file filetype error (internal case)
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_filename("originalFile-seq1");
        snapInfo.set_seqnum(1);
        snapInfo.set_filetype(FileType::INODE_APPENDFILE);
        snapShotFiles.push_back(snapInfo);

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        EXPECT_EQ(curvefs_->DeleteFileSnapShotFile("/originalFile", 1, nullptr),
            StatusCode::KInternalError);
    }
    {
        // delete storage error
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_filename("originalFile-seq1");
        snapInfo.set_seqnum(1);
        snapInfo.set_filetype(FileType::INODE_SNAPSHOT_PAGEFILE);
        snapInfo.set_filestatus(FileStatus::kFileCreated);
        snapShotFiles.push_back(snapInfo);

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, PutFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::InternalError));

        EXPECT_EQ(curvefs_->DeleteFileSnapShotFile("/originalFile", 1, nullptr),
            StatusCode::KInternalError);
    }
    {
        // delete snapshot ok
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_filename("originalFile-seq1");
        snapInfo.set_seqnum(1);
        snapInfo.set_filetype(FileType::INODE_SNAPSHOT_PAGEFILE);
        snapInfo.set_filestatus(FileStatus::kFileCreated);
        snapShotFiles.push_back(snapInfo);

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, PutFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        EXPECT_CALL(*mockSnapShotCleanManager_,
            SubmitDeleteSnapShotFileJob(_, _))
        .Times(1)
        .WillOnce(Return(true));

        EXPECT_EQ(curvefs_->DeleteFileSnapShotFile("/originalFile", 1, nullptr),
            StatusCode::kOK);
    }
    {
        //  message the snapshot delete manager error, return error
        FileInfo originalFile;
        originalFile.set_id(1);
        originalFile.set_seqnum(1);
        originalFile.set_filename("originalFile");
        originalFile.set_fullpathname("/originalFile");
        originalFile.set_filetype(FileType::INODE_PAGEFILE);

        EXPECT_CALL(*storage_, GetFile(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(originalFile),
            Return(StoreStatus::OK)));

        std::vector<FileInfo> snapShotFiles;
        FileInfo snapInfo;
        snapInfo.set_filename("originalFile-seq1");
        snapInfo.set_seqnum(1);
        snapInfo.set_filetype(FileType::INODE_SNAPSHOT_PAGEFILE);
        snapInfo.set_filestatus(FileStatus::kFileCreated);
        snapShotFiles.push_back(snapInfo);

        EXPECT_CALL(*storage_, ListFile(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(snapShotFiles),
                Return(StoreStatus::OK)));

        EXPECT_CALL(*storage_, PutFile(_, _))
        .Times(1)
        .WillOnce(Return(StoreStatus::OK));

        EXPECT_CALL(*mockSnapShotCleanManager_,
            SubmitDeleteSnapShotFileJob(_, _))
        .Times(1)
        .WillOnce(Return(false));

        EXPECT_EQ(curvefs_->DeleteFileSnapShotFile("/originalFile", 1, nullptr),
            StatusCode::KInternalError);
    }
}

TEST_F(CurveFSTest, CheckSnapShotFileStatus) {
    // may be not needed
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}

}  // namespace mds
}  // namespace curve
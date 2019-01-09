/*
 * Project: curve
 * Created Date: Tuesday September 25th 2018
 * Author: hzsunjianliang
 * Copyright (c) 2018 netease
 */
#include <algorithm>
#include <vector>
#include "src/mds/nameserver2/namespace_service.h"
#include "src/mds/nameserver2/curvefs.h"

namespace curve {
namespace mds {

void NameSpaceService::CreateFile(::google::protobuf::RpcController* controller,
                       const ::curve::mds::CreateFileRequest* request,
                       ::curve::mds::CreateFileResponse* response,
                       ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
        << ", CreateFile request, filename = " << request->filename()
        << ", filetype = " << request->filetype()
        << ", filelength = " << request->filelength();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission
    StatusCode retCode = kCurveFS.CreateFile(request->filename(),
            request->filetype(), request->filelength());
    if (retCode != StatusCode::kOK)  {
        response->set_statuscode(retCode);
        // TODO(hzsunjianliang): check if we should really print error here
        LOG(ERROR) << "logid = " << cntl->log_id()
            << ", CreateFile fail, filename = " <<  request->filename()
            << ", statusCode = " << retCode;
        return;
    } else {
        response->set_statuscode(StatusCode::kOK);
        LOG(INFO) << "logid = " << cntl->log_id()
            << ", CreateFile ok, filename = " << request->filename();
    }
    return;
}

void NameSpaceService::GetFileInfo(
                        ::google::protobuf::RpcController* controller,
                        const ::curve::mds::GetFileInfoRequest* request,
                        ::curve::mds::GetFileInfoResponse* response,
                        ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(ERROR) << "logid = " << cntl->log_id()
        << ", GetFileInfo request, filename = " << request->filename();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission
    StatusCode retCode;
    retCode = kCurveFS.GetFileInfo(request->filename(),
        response->mutable_fileinfo());

    if (retCode != StatusCode::kOK)  {
        response->set_statuscode(retCode);
        LOG(ERROR) << "logid = " << cntl->log_id()
            << ", GetFileInfo fail, filename = " <<  request->filename()
            << ", statusCode = " << retCode;
        return;
    } else {
        response->set_statuscode(StatusCode::kOK);
        LOG(INFO) << "logid = " << cntl->log_id()
            << ", GetFileInfo ok, filename = " << request->filename();
    }
    return;
}

void NameSpaceService::GetOrAllocateSegment(
                    ::google::protobuf::RpcController* controller,
                    const ::curve::mds::GetOrAllocateSegmentRequest* request,
                    ::curve::mds::GetOrAllocateSegmentResponse* response,
                    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
        << ", GetOrAllocateSegment request, filename = " << request->filename()
        << ", offset = " << request->offset() << ", allocateTag = "
        << request->allocateifnotexist();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission

    StatusCode retCode;
    retCode = kCurveFS.GetOrAllocateSegment(request->filename(),
                request->offset(),
                request->allocateifnotexist(),
                response->mutable_pagefilesegment());

    if (retCode != StatusCode::kOK)  {
        response->set_statuscode(retCode);
        LOG(ERROR) << "logid = " << cntl->log_id()
            << ", GetOrAllocateSegment fail, filename = "
            <<  request->filename()
            << ", offset = " << request->offset()
            << ", allocateTag = " << request->allocateifnotexist()
            << ", statusCode = " << retCode;
    } else {
        response->set_statuscode(StatusCode::kOK);
        LOG(INFO) << "logid = " << cntl->log_id()
            << ", GetOrAllocateSegment ok, filename = " << request->filename()
            << ", offset = " << request->offset()
            << ", allocateTag = " << request->allocateifnotexist();
    }
    return;
}

void NameSpaceService::DeleteSegment(
                         ::google::protobuf::RpcController* controller,
                         const ::curve::mds::DeleteSegmentRequest* request,
                         ::curve::mds::DeleteSegmentResponse* response,
                         ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
        << ", DeleteSegment request, filename = " << request->filename()
        << ", offset = " << request->offset();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission
    StatusCode retCode = kCurveFS.DeleteSegment(request->filename(),
            request->offset());
    if (retCode != StatusCode::kOK)  {
        response->set_statuscode(retCode);
        LOG(ERROR) << "logid = " << cntl->log_id()
            << ", DeleteSegment fail, filename = " << request->filename()
            << ", offset = " << request->offset()
            << ", statusCode = " << retCode;
    } else {
        response->set_statuscode(StatusCode::kOK);
        LOG(INFO) << "logid = " << cntl->log_id()
            << ", DeleteSegment ok, filename = " << request->filename()
            << ", offset = " << request->offset();
    }

    return;
}
void NameSpaceService::RenameFile(::google::protobuf::RpcController* controller,
                         const ::curve::mds::RenameFileRequest* request,
                         ::curve::mds::RenameFileResponse* response,
                         ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
        << ", RenameFile request, oldfilename = " << request->oldfilename()
        << ", newfilename = " << request->newfilename();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission
    StatusCode retCode = kCurveFS.RenameFile(request->oldfilename(),
            request->newfilename());
    if (retCode != StatusCode::kOK)  {
        response->set_statuscode(retCode);
        LOG(ERROR) << "logid = " << cntl->log_id()
            << ", RenameFile fail, oldfilename = " << request->oldfilename()
            << ", newfilename = " << request->newfilename()
            << ", statusCode = " << retCode;
    } else {
        response->set_statuscode(StatusCode::kOK);
        LOG(INFO) << "logid = " << cntl->log_id()
            << ", RenameFile ok, oldFileName = " << request->oldfilename()
            << ", newFileName = " << request->newfilename();
    }
    return;
}

void NameSpaceService::ExtendFile(::google::protobuf::RpcController* controller,
                    const ::curve::mds::ExtendFileRequest* request,
                    ::curve::mds::ExtendFileResponse* response,
                    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
              << ", ExtendFile request, filename = " << request->filename()
              << ", newsize = " << request->newsize();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission
    StatusCode retCode = kCurveFS.ExtendFile(request->filename(),
           request->newsize());
    if (retCode != StatusCode::kOK)  {
       response->set_statuscode(retCode);
       LOG(ERROR) << "logid = " << cntl->log_id()
                  << ", ExtendFile fail, filename = " << request->filename()
                  << ", newsize = " << request->newsize()
                  << ", statusCode = " << retCode;
    } else {
       response->set_statuscode(StatusCode::kOK);
       LOG(INFO) << "logid = " << cntl->log_id()
           << ", ExtendFile ok, filename = " << request->filename()
           << ", newsize = " << request->newsize();
    }

    return;
}

void NameSpaceService::CreateSnapShot(
                        ::google::protobuf::RpcController* controller,
                       const ::curve::mds::CreateSnapShotRequest* request,
                       ::curve::mds::CreateSnapShotResponse* response,
                       ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
              << ", CreateSnapShot request, filename = "
              << request->filename();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission
    StatusCode retCode = kCurveFS.CreateSnapShotFile(request->filename(),
                                    response->mutable_snapshotfileinfo());
    if (retCode != StatusCode::kOK)  {
       response->set_statuscode(retCode);
       LOG(ERROR) << "logid = " << cntl->log_id()
                  << ", CreateSnapShot fail, filename = " << request->filename()
                  << ", statusCode = " << retCode;
    } else {
       response->set_statuscode(StatusCode::kOK);
       LOG(INFO) << "logid = " << cntl->log_id()
           << ", CreateSnapShot ok, filename = " << request->filename()
           << ", seq = " << response->snapshotfileinfo().seqnum();
    }
    return;
}

void NameSpaceService::ListSnapShot(
                    ::google::protobuf::RpcController* controller,
                    const ::curve::mds::ListSnapShotFileInfoRequest* request,
                    ::curve::mds::ListSnapShotFileInfoResponse* response,
                    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
              << ", ListSnapShot request, filename = "
              << request->filename();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission
    std::vector<FileInfo> snapShotFiles;
    StatusCode retCode = kCurveFS.ListSnapShotFile(request->filename(),
                                &snapShotFiles);

    if (retCode == StatusCode::kOK) {
        auto size =  request->seq_size();
        for (int i = 0; i != size; i++) {
            auto tofindseq = request->seq(i);
            LOG(INFO) << "tofindseq = " << tofindseq;
            auto iter =
                std::find_if(snapShotFiles.begin(), snapShotFiles.end(),
                [&](const FileInfo &val){ return val.seqnum() == tofindseq; });

            if (iter != snapShotFiles.end()) {
                FileInfo *fileinfo = response->add_fileinfo();
                fileinfo->CopyFrom(*iter);
            }
        }
    }

    if (retCode != StatusCode::kOK)  {
       response->set_statuscode(retCode);
       LOG(ERROR) << "logid = " << cntl->log_id()
                  << ", ListSnapShot fail, filename = " << request->filename()
                  << ", statusCode = " << retCode;
    } else {
       response->set_statuscode(StatusCode::kOK);
       LOG(INFO) << "logid = " << cntl->log_id()
                  << ", ListSnapShot ok, filename = " << request->filename()
                  << ", statusCode = " << retCode;
    }
    return;
}

void NameSpaceService::DeleteSnapShot(
                    ::google::protobuf::RpcController* controller,
                    const ::curve::mds::DeleteSnapShotRequest* request,
                    ::curve::mds::DeleteSnapShotResponse* resp,
                    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
              << ", DeleteSnapShot request, filename = "
              << request->filename()
              << ", seq = " << request->seq();

    // TODO(hzsunjialiang): lock the filepath&name and do check permission

    auto asynEntity =
        std::make_shared<AsyncDeleteSnapShotEntity>(resp,
                        request, controller, nullptr);

    StatusCode retCode =  kCurveFS.DeleteFileSnapShotFile(request->filename(),
                                    request->seq(), asynEntity);

    if (retCode != StatusCode::kOK) {
        resp->set_statuscode(retCode);
        LOG(ERROR) << "logid = " << cntl->log_id()
                  << ", DeleteSnapShot fail, filename = " << request->filename()
                  << ", seq = " << request->seq()
                  << ", statusCode = " << retCode;
        return;
    }
    asynEntity->SetClosure(doneGuard.release());
    // release the rpc return to the async back ground process
    return;
}

void NameSpaceService::CheckSnapShotStatus(
                    ::google::protobuf::RpcController* controller,
                    const ::curve::mds::CheckSnapShotStatusRequest* request,
                    ::curve::mds::CheckSnapShotStatusResponse* response,
                    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "logid = " << cntl->log_id()
              << ", CheckSnapShotStatus not support yet, filename = "
              << request->filename()
              << ", seqnum" << request->seq();

    response->set_statuscode(StatusCode::kNotSupported);
    return;
}

void NameSpaceService::GetSnapShotFileSegment(
                    ::google::protobuf::RpcController* controller,
                    const ::curve::mds::GetOrAllocateSegmentRequest* request,
                    ::curve::mds::GetOrAllocateSegmentResponse* response,
                    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    if ( !request->has_seqnum() ) {
        LOG(ERROR) << "logid = " << cntl->log_id()
              << ", GetSnapShotFileSegment, filename = "
              << request->filename()
              << ", seqnum not found";
    }

    LOG(INFO) << "logid = " << cntl->log_id()
              << ", GetSnapShotFileSegment request, filename = "
              << request->filename()
              << " offset = " << request->offset()
              << ", seqnum = " << request->seqnum();

    StatusCode retCode = kCurveFS.GetSnapShotFileSegment(request->filename(),
                        request->seqnum(),
                        request->offset(),
                        response->mutable_pagefilesegment());
    if (retCode  != StatusCode::kOK) {
        response->set_statuscode(retCode);
        LOG(ERROR) << "logid = " << cntl->log_id()
            << ", GetSnapShotFileSegment fail, filename = "
            <<  request->filename()
            << ", offset = " << request->offset()
            << ", seqnum = " << request->seqnum()
            << ", statusCode = " << retCode;
    } else {
        response->set_statuscode(StatusCode::kOK);
        LOG(INFO) << "logid = " << cntl->log_id()
            << ", GetSnapShotFileSegment ok, filename = "
            <<  request->filename()
            << ", offset = " << request->offset()
            << ", seqnum = " << request->seqnum()
            << ", statusCode = " << retCode;
    }
    return;
}


}  // namespace mds
}  // namespace curve
